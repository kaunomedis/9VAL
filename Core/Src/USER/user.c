/* ***********************************************
**                                              **
** 40mA   RTC clock V1.0 www.vabolis.lt (c)2023 **
**                                              **
************************************************ */

#include "user.h"


#include "rtc.c"
#include "circular_buffer.c"
#include "oled/ssd1306_oled_i2c.c"

extern IWDG_HandleTypeDef hiwdg;
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim1;

unsigned char invertuotas=0;
uint32_t bootsecons;

#define BUFFER_SIZE 64
unsigned char testas[BUFFER_SIZE];

CCBuf cc; //struktura

volatile char seconds;


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
	
	//HAL_TIM_Base_Start(&htim1);
	
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	//HAL_TIM_Base_Start_IT(&htim1);
__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,5*255);
	//HAL_RTCEx_SetSmoothCalib(&hrtc,0,0,10); // du nuliai nes F1 nera. paskutinis- skaicius 0-7F, tik mazina greiti. 127=314sekundziu per 30d.

RTC_DateTypeDef dienos= {0};
HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
bootsecons=currTime.Hours*3600+currTime.Minutes*60+currTime.Seconds+(dienos.Date-1)*86400;




SSD1306_Init();
HAL_Delay(500);
SSD1306_Init();
SSD1306_clear(0);
SSD1306_move(0,0);
//HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
SSD1306_puts("WWW.VABOLIS.LT");
HAL_Delay(500);
SSD1306_dim(1);
HAL_Delay(500);
SSD1306_clear(0);


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
	
	show_time();
	
seconds++;
__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,seconds*255);
}


/*
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)  //16kHz?
{
  if (htim == &htim1 )
  {
  }
}
*/


void show_time(void)
{
unsigned char font[]={0x00, 0x00, 0xe7, 0xe7, 0xe7, 0xe7, 0x00, 0x00};
uint32_t nowsecons, uptime;
char text[9];

RTC_DateTypeDef dienos;

HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
//HAL_Delay(5);

HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
nowsecons=currTime.Hours*3600+currTime.Minutes*60+currTime.Seconds+(dienos.Date-1)*86400;

if(nowsecons>bootsecons){uptime=nowsecons-bootsecons;} else {uptime=0;} //error!



SSD1306_bigdigit(0,1+0,currTime.Hours/10);
SSD1306_bigdigit(0,1+2,currTime.Hours%10);
SSD1306_bigdigit(0,1+5,currTime.Minutes/10);
SSD1306_bigdigit(0,1+7,currTime.Minutes%10);
SSD1306_bigdigit(0,1+10,currTime.Seconds/10);
SSD1306_bigdigit(0,1+12,currTime.Seconds%10);

SSD1306_move(1, 1+4);

SSD1306_put_tile(font,8);

SSD1306_move(1, 1+9);
SSD1306_put_tile(font,8);

currTime.Hours=uptime/3600;
currTime.Minutes=(uptime-currTime.Hours*3600)/60;
currTime.Seconds=uptime-currTime.Hours*3600-currTime.Minutes*60;



text[0]=(currTime.Hours)/10+'0';
text[1]=(currTime.Hours)%10+'0';
text[2]=':';	

text[3]=(currTime.Minutes)/10+'0';
text[4]=(currTime.Minutes)%10+'0';
text[5]=':';	


text[6]=(currTime.Seconds)/10+'0';
text[7]=(currTime.Seconds)%10+'0';
text[8]=0;

//itoa(uptime,text,10);



SSD1306_move(3,4);
SSD1306_puts(text);

if(uptime>3*3600) {invertuotas=1;}

if(invertuotas>0){SSD1306_invert(); invertuotas--;} else {SSD1306_normal();}

HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

