/* ***********************************************
**                                              **
** 40mA   RTC clock V1.0 www.vabolis.lt (c)2023 **
**                                              **
************************************************ */

#include "user.h"


#include "rtc.c"
#include "circular_buffer.c"

extern IWDG_HandleTypeDef hiwdg;
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim1;


#define BUFFER_SIZE 64
unsigned char testas[BUFFER_SIZE];

CCBuf cc; //struktura




void user_usb_rx(uint8_t* Buf, uint32_t *Len)
{
unsigned char i,a;
for (i=0;i<*Len;i++)
	{
		a=Buf[i];
		if (a>10) circle_push(&cc , a);
	}
}

void user_init(void)
{
	cc.buffer=testas;
	circle_reset(&cc,BUFFER_SIZE);			//init circle buffer
	rtc_int_init();
	
	HAL_TIM_Base_Start_IT(&htim1);

	//HAL_RTCEx_SetSmoothCalib(&hrtc,0,0,10); // du nuliai nes F1 nera. paskutinis- skaicius 0-7F, tik mazina greiti. 127=314sekundziu per 30d.
}



void commandcom(char * txt) // network (UART,USB) command interpreter
{

if (txt[0] !='A' || txt[1]!='T') return;

	switch(txt[2])
	{
		case 'T':
			rtc_set_time_text(txt+3);
		break;
		case 'D':
			rtc_set_date_text(txt+3);		
		break;
		case 'I':
			CDC_Transmit_FS((uint8_t*) "9H CLOCK\r\n(c)2023 Vabolis.lt\r\n",33);
		break;
		case 'A':
			__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,55*255);
		break;
	}
}


void user_loop(void)
{
char a;
char txt[32];
unsigned char txtp=0;

while(1)
	{
	HAL_IWDG_Refresh(&hiwdg); //watchdogas
	while(circle_available(&cc)>0) 
		{
			a=circle_pull(&cc);
			
			if (a>=' ' && a<='z')
			{
				txt[txtp]=a;
				txtp++;
				txt[txtp]=0;
				if (txtp >30) {txtp=0;}
			}
			else if (a<' ')
			{
				if (txtp>2)commandcom(txt);
				txt[0]=0;
				txtp=0;
			}
		}
	}
} /* be isejimo is loop! */



void user_seconds_job(void)
{
	HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);

	HAL_GPIO_TogglePin(GPIOA, LED_Pin);

}


/*
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)  //16kHz?
{
  if (htim == &htim1 )
  {
  }
}
*/


