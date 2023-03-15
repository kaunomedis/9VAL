/* ********************************************* */
/* (c)2022 by www.vabolis.lt, Kaunas             */
/**
  **********************************************
  * @file           : rtc.c
  * @version        : 1.0
  * @brief          : generic RTC clock and calendar for STM32
**/  
/* ********************************************* */

#include "rtc.h"
#include "user.h"
#include "usbd_cdc_if.h"

//#include "epoch.c"

extern RTC_HandleTypeDef hrtc;


// ****** pacioje pradzioje, globalus laikas. Galima persideklaruoti, bet ar verta?

RTC_TimeTypeDef currTime = {0};
RTC_TimeTypeDef oldTime = {0xFF,0xFF,0xFF}; //del refresh
volatile unsigned char LastHour=255;

/* instrukcijos kurias reikia ikelti i main.c */

//-  /* USER CODE BEGIN Check_RTC_BKUP */
//-  
//-  // CIA TIKRINAM AR RTC ir kiti RAM baitai isliko gyvi.
//-    if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1)!= 0x5051)
//-	{
//-  /* USER CODE END Check_RTC_BKUP */


//- /* USER CODE BEGIN RTC_Init 2 */
//-	} // Kitame user code virsuje yra IF komanda. Cia ji uzsidaro.
//-  // LAIKAS BUVO ISSAUGOTAS, NES USER REGISTRAS TURI MAGIC skaiciu 0x5051
//- 
//- /* read RTC for time */
//-	HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
//- /* read time seconds for epoch */
//- // uint32_t nowsecons=currTime.Hours*3600+currTime.Minutes*60+currTime.Seconds+(dienos.Date-1)*86400;
//-
//- /* set time to RTC */
//-	if (HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN) != HAL_OK)
//-	{
//-	Error_Handler();
//-	}
//-
//- // DEMESIO! STM32F103 NETURI KALENDORIAUS ir panasu, kad neturi epoch skaitliuko.
//-	//RTC_DateTypeDef dienos= {0};
//-	//HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
//-	HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
//-
//- //Cia irasom MAGIC skaiciu. Jis po pilno reseto ir RTC mirties turi neislikti.
//-	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x5051);//Write data to the specified backing area register
//-
//- //HAL_RTCEx_SetSmoothCalib(&hrtc,0,0,10);
//-   /* USER CODE END RTC_Init 2 */


// sunaikina sena laika. Naudojama LCD grafikos spartinimui
void rtc_clean(void)
{
	oldTime.Hours = 60;
	oldTime.Minutes = 60;
	oldTime.Seconds = 60;
}




// teksto filtras laiko ir datos nustatymui.
void filter_string(char *Buf)
{
int i=0;
int j=0;

for(i=0;Buf[i] != 0; i++)
	{
	while(!((Buf[i]>='0' && Buf[i]<='9') || Buf[i]==':' || Buf[i]=='.') && !(Buf[i]==0))
		{
			for(j=i;Buf[j] !=0; ++j)
				{
					Buf[j]=Buf[j+1];
				}
			Buf[j]=0;
		}
	}
}

//nustatyti kalendoriu.
void rtc_set_date_text(char * Buf)
{
filter_string(Buf);
//22.12.23

RTC_DateTypeDef dienos;

if(strlen(Buf)<8) return;
	char delim[] = ".";

	char *ptr = strtok(Buf, delim);
	dienos.Year=atoi(ptr);
	
	ptr = strtok(NULL, delim);
	dienos.Month=atoi(ptr);
	
	ptr = strtok(NULL, delim);
	dienos.Date=atoi(ptr);
	
 if (HAL_RTC_SetDate(&hrtc, &dienos, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  
  rtc_backup_date();
}

//nustatyti laikrodi
void rtc_set_time_text(char * Buf)
{
// echo ATT%TIME% >COMx
//ATT19:14:32.94


filter_string(Buf);

if(strlen(Buf)<8) return;

	char delim[] = ":";

	char *ptr = strtok(Buf, delim);
	currTime.Hours=atoi(ptr);
	
	ptr = strtok(NULL, delim);
	currTime.Minutes=atoi(ptr);
	
	ptr = strtok(NULL, delim);
	currTime.Seconds=atoi(ptr);
	
 if (HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

}

// nuskaityti laika is hardwares.
void rtc_update(void)
{
	HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
}

//rtc->tekstas
void rtc_time_string(char * text)
{
	HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
	text[0]=(currTime.Hours)/10+'0';
	text[1]=(currTime.Hours)%10+'0';
	text[2]=':';	

	text[3]=(currTime.Minutes)/10+'0';
	text[4]=(currTime.Minutes)%10+'0';
	text[5]=':';	

	text[6]=(currTime.Seconds)/10+'0';
	text[7]=(currTime.Seconds)%10+'0';
	text[8]=0;
}
void rtc_date_string(char * text)
{
	RTC_DateTypeDef dienos;
	HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);

	text[0]=(dienos.Year)/10+'0';
	text[1]=(dienos.Year)%10+'0';
	text[2]='.';
	text[3]=(dienos.Month)/10+'0';
	text[4]=(dienos.Month)%10+'0';
	text[5]='.';
	text[6]=(dienos.Date)/10+'0';
	text[7]=(dienos.Date)%10+'0';
	text[8]=0;
}

void rtc_backup_date(void)
{
RTC_DateTypeDef dienos;
	if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1)== 0x5051) //magic number, valid backup
	{
		HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
		HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, (uint16_t) (dienos.Year<<8) + dienos.Month);	
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, (uint16_t) (dienos.Date<<8) + currTime.Hours);	
	}
}

void rtc_check_screwd_calendar(void)
{
RTC_DateTypeDef dienos;
	if(HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR1)== 0x5051) //magic number, valid backup
	{
		HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
		HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
		uint32_t now_is=currTime.Hours+dienos.Date*24+dienos.Month*31*24+dienos.Year*366*24;
		
		uint32_t tmp=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR2);
		uint32_t tmp2=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR3);	
	
		uint32_t now_was=(tmp>>8)*366*24+(tmp & 0x000F)*31*24+(tmp2>>8)*24+(tmp2 & 0xFF);
		if(now_was > now_is)
			{
			dienos.Year=(tmp >> 8);
			dienos.Month=tmp & 0x0F;
			dienos.Date=(tmp2 >> 8);
			 if (HAL_RTC_SetDate(&hrtc, &dienos, RTC_FORMAT_BIN) != HAL_OK)
			  {
				Error_Handler();
			  }
			 rtc_backup_date();
			}
	}
}


/* INT INT INT INT INT INT INT INT INT INT INT INT INT INT INT INT */

/* ijungti RTC pertraukimus */
void rtc_int_init(void)
{
	__HAL_RTC_SECOND_ENABLE_IT(&hrtc,RTC_IT_SEC); //turn on RTC clock seconds interrupt
}


/**
  * @brief  Second event callback.
**/
/* kas sekunde, turi buti ijungtas INT */
void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc)
{
	user_seconds_job();
	//if(LastHour != currTime.Hours) {rtc_backup_date(); LastHour=currTime.Hours;}
}