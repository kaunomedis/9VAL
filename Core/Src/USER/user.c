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
extern TIM_HandleTypeDef htim2;

unsigned char invertuotas=0;
//uint32_t bootsecons;

#define BUFFER_SIZE 64
unsigned char testas[BUFFER_SIZE];

CCBuf cc; //struktura

volatile char seconds;
volatile char buttons;
volatile char old_buttons;
volatile char mode;
volatile uint32_t start_hour;
volatile uint32_t start_minutes;
/* program state:

0- normal,boot, time
1- setup hour
2- setup minutes
3- setup start hour
4- setup start minute

255-OLED saver
*/
#define MODE_DELAY 30
#define SLEEP_DELAY 300

volatile unsigned char mode_delay=MODE_DELAY;
volatile uint32_t sleep_delay=SLEEP_DELAY;

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

	HAL_IWDG_Refresh(&hiwdg); //watchdogas

	
	SSD1306_Init();
	HAL_IWDG_Refresh(&hiwdg); //watchdogas
	HAL_Delay(500);
	SSD1306_Init();
	SSD1306_clear(0);
	SSD1306_move(0,0);
	HAL_IWDG_Refresh(&hiwdg); //watchdogas

	SSD1306_puts("WWW.VABOLIS.LT");
	HAL_Delay(500);
	SSD1306_dim(1);
	HAL_IWDG_Refresh(&hiwdg); //watchdogas
	HAL_Delay(500);
	SSD1306_clear(0);
	
	rtc_int_init();
		
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	HAL_TIM_Base_Start_IT 	(&htim2);
	__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,5*255);
	//HAL_RTCEx_SetSmoothCalib(&hrtc,0,0,10); // du nuliai nes F1 nera. paskutinis- skaicius 0-7F, tik mazina greiti. 127=314sekundziu per 30d.

	RTC_DateTypeDef dienos= {0};
	HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
	HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
	//bootsecons=currTime.Hours*3600+currTime.Minutes*60+currTime.Seconds+(dienos.Date-1)*86400;

	start_hour=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR4);
	start_minutes=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR5);
}

void ReadPins(void)
{
if(!HAL_GPIO_ReadPin(GPIOB, BTN1_Pin)){buttons=buttons | 1;} else {buttons=buttons & 0xFE;}
if(!HAL_GPIO_ReadPin(GPIOB, BTN2_Pin)){buttons=buttons | 2;} else {buttons=buttons & 0xFD;}
if(!HAL_GPIO_ReadPin(GPIOB, BTN3_Pin)){buttons=buttons | 4;} else {buttons=buttons & 0xFB;}
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
uint32_t nowseconds, starttime,pwm;
	show_time();
	nowseconds=currTime.Hours*3600+currTime.Minutes*60+currTime.Seconds; //+(dienos.Date-1)*86400;
	starttime=start_hour*3600+start_minutes*60; //+(dienos.Date-1)*86400;
	
	
	if ((nowseconds > starttime) && (nowseconds < starttime+32400))
		{
		pwm=32400+starttime-nowseconds; //9h+starttime-now. Reziuose gaunasi 32400-0
		
		
		}
		else
		{
		pwm=0;
		}
		

__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,pwm);
}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)  //2Hz?
{
char text[5];
unsigned char tmp;

  if (htim == &htim2 )
  {
  	ReadPins();
	HAL_GPIO_TogglePin(GPIOA, LED_Pin);
	if (buttons==0 && mode_delay>0) {mode_delay--;}
	if (mode_delay==1) {mode=0; SSD1306_move(3,0); SSD1306_puts("                "); mode_delay=0; sleep_delay=SLEEP_DELAY;}
	if (mode_delay==0 && sleep_delay>0) {sleep_delay--;}
	if (sleep_delay==0) {mode=5; SSD1306_command1(SSD1306_DISPLAYOFF);}
	if (buttons != old_buttons)
		{
		old_buttons=buttons;	
		
		text[2]=' ';
		text[3]=' ';
		text[4]=0;
		
		if (buttons==1)
			{
			mode++;
			mode_delay=MODE_DELAY;
			sleep_delay=SLEEP_DELAY;
			if (mode>4) {mode=0; SSD1306_command1(SSD1306_DISPLAYON);}
			}
		else if (buttons==6) {HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN); mode=0; currTime.Seconds=0; HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);show_time();} //du mygtukai=00 sekundziu
			//########## 
		else {
			switch( mode ) 
				{
					case 0x00: //normal mode
					SSD1306_move(3,0); SSD1306_puts("                ");
					break;
					case 0x01: //setup hour
						
						__disable_irq();
						HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
						if(buttons==2) {
											tmp=currTime.Hours;
											tmp++; if (tmp>23) tmp=0;
											currTime.Hours=(uint8_t)tmp;
											HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
											show_time(); mode_delay=MODE_DELAY;
										}
						if(buttons==4) {
											tmp=currTime.Hours;
											tmp--;
											if(tmp == 255) {tmp=23;}
											currTime.Hours=(uint8_t)tmp;
											HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
											show_time(); mode_delay=MODE_DELAY;
										}
						SSD1306_move(3,0); SSD1306_puts("Setting hours ");
						__enable_irq();
					break;
					case 0x02: //setup minute
						__disable_irq();
						HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
						if(buttons==2) {
											tmp=currTime.Minutes;
											tmp++; if(tmp>59) tmp=0;
											currTime.Minutes=(uint8_t)tmp;
											HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
											show_time(); mode_delay=MODE_DELAY;
										}
						if(buttons==4) {
											tmp=currTime.Minutes;
											tmp--; if(tmp ==255 ) {tmp=59;}
											currTime.Minutes=(uint8_t)tmp;
											HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
											show_time(); mode_delay=MODE_DELAY;
										}
						SSD1306_move(3,0); SSD1306_puts("Setting mins  ");
						__enable_irq();
					break;
					case 0x03: //setup start hour
						if(buttons==2) {
											start_hour=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR4);
											start_hour++;
											if (start_hour > 23U) {start_hour=0;}
											HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, start_hour);
											mode_delay=MODE_DELAY;
										}
						if(buttons==4) {
											start_hour=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR4);
											start_hour--;
											if(start_hour > 25U) {start_hour=23;}
											HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, start_hour);
											mode_delay=MODE_DELAY;
										}
						text[0]=start_hour/10+'0'; text[1]=start_hour % 10+'0';
						SSD1306_move(3,0); SSD1306_puts("End hour:"); SSD1306_puts(text);
					break;
					case 0x04: //setup start minute
						if(buttons==2) {
											start_minutes=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR5);
											start_minutes++;
											if (start_minutes > 59U) {start_minutes=0;}
											HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, start_minutes);
											mode_delay=MODE_DELAY;
										}
						if(buttons==4) {
											start_minutes=HAL_RTCEx_BKUPRead(&hrtc,RTC_BKP_DR5);
											start_minutes--;
											if(start_minutes > 60U) {start_minutes=59;}
											HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, start_minutes);
											mode_delay=MODE_DELAY;
										}
						text[0]=start_minutes/10+'0'; text[1]=start_minutes % 10+'0';
						SSD1306_move(3,0); SSD1306_puts("End min:"); SSD1306_puts(text);
					break;
					case 0x05: //xxx
					break;
				}
			}

		}
		
		
	//text[0]=mode+'0';
	//text[1]=0;
	
	//SSD1306_puts(text);
		
		
  }
}



void show_time(void)
{
unsigned char font[]={0x00, 0x00, 0xe7, 0xe7, 0xe7, 0xe7, 0x00, 0x00};

//char text[9];

//RTC_DateTypeDef dienos;

//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
//HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
//HAL_Delay(5);

HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);

//if(nowsecons>bootsecons){uptime=nowsecons-bootsecons;} else {uptime=0;} //error!



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

//currTime.Hours=uptime/3600;
//currTime.Minutes=(uptime-currTime.Hours*3600)/60;
//currTime.Seconds=uptime-currTime.Hours*3600-currTime.Minutes*60;


/*
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
*/
//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

