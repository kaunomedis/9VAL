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
volatile uint16_t start_hour;
volatile uint16_t start_minutes;
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

#define ST_H RTC_BKP_DR6
#define ST_M RTC_BKP_DR7

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

	//RTC_DateTypeDef dienos= {0};
	//HAL_RTC_GetDate(&hrtc, &dienos, RTC_FORMAT_BIN);
	

    HAL_PWR_EnableBkUpAccess();
	
	
	HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);
	//bootsecons=currTime.Hours*3600+currTime.Minutes*60+currTime.Seconds+(dienos.Date-1)*86400;

	start_hour=HAL_RTCEx_BKUPRead(&hrtc,ST_H);
	start_minutes=HAL_RTCEx_BKUPRead(&hrtc,ST_M);
}

void ReadPins(void)
{
if(!HAL_GPIO_ReadPin(GPIOB, BTN3_Pin)){buttons=buttons | 1;} else {buttons=buttons & 0xFE;}
if(!HAL_GPIO_ReadPin(GPIOB, BTN1_Pin)){buttons=buttons | 2;} else {buttons=buttons & 0xFD;}
if(!HAL_GPIO_ReadPin(GPIOB, BTN2_Pin)){buttons=buttons | 4;} else {buttons=buttons & 0xFB;}
}

void set_start_text(char * Buf)
{
filter_string(Buf);
//09:00


if(strlen(Buf)<5) return;
	char delim[] = ":";

	char *ptr = strtok(Buf, delim);
	start_hour=atoi(ptr);
	
	ptr = strtok(NULL, delim);
	start_minutes=atoi(ptr);
	
}

void user_usb_tx(uint8_t* Buf, uint16_t Len)
{
uint8_t result=USBD_BUSY;
unsigned char retry=5;

while(result !=USBD_OK && retry>1)
	{
		result = CDC_Transmit_FS(Buf, Len);
		if (result==USBD_BUSY) HAL_Delay(10); //CDC_HS_BINTERVAL or CDC_FS_BINTERVAL
		retry--;
	}
}

void show_help(void)
{
char txt[]="\r\n?\r\n9h Clock. Use AT commands to setup.\r\n ATT13:00:05 -to setup time. ATT only shows current time.\r\n ATS09:00 -new start time.\r\n ATI -information.\r\n ATAxxx -debug PWM.\r\n";
user_usb_tx((uint8_t*) txt,80);
}

void commandcom(char * txt) // network (UART,USB) command interpreter
{

if (txt[0] !='A' || txt[1]!='T') return;

	switch(txt[2])
	{
		case 'T':
			rtc_set_time_text(txt+3);
			rtc_time_string(txt); txt[8]='\r'; txt[9]='\n';
			CDC_Transmit_FS((uint8_t*) txt,10);
		break;
		case 'D':
			//rtc_set_date_text(txt+3);	
			//rtc_date_string(txt); txt[8]='\r'; txt[9]='\n';
			//CDC_Transmit_FS((uint8_t*) txt,10);
		break; 
		case 'I':
			CDC_Transmit_FS((uint8_t*) "9H CLOCK\r\n(c)2023 Vabolis.lt\r\n ",30);
		break;
		case 'S':
			set_start_text(txt+3);
			Write_Start_stop();
			
			CDC_Transmit_FS((uint8_t*) "New start time set.\r\n ",21);
		break;
		case 'A':
			HAL_IWDG_Refresh(&hiwdg);
			__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,atoi(txt+3));
		break;
		default:
			show_help();
		break;
	}
	//CDC_Transmit_FS((uint8_t*) "\r\n",2);
	mode=0;
	mode_delay=MODE_DELAY;
	sleep_delay=SLEEP_DELAY;
	SSD1306_command1(SSD1306_DISPLAYON);
	old_buttons=255;
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
		pwm=(32400+starttime-nowseconds); //9h+starttime-now. Reziuose gaunasi 32400-0
		pwm=(pwm*214)/1000+1520;
		}
		else
		{
		pwm=0;
		}

__HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,pwm);
}

void Write_Start_stop(void)
{
	HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKP_CLK_ENABLE(); //sitas neinicializuotas!
	HAL_RTCEx_BKUPWrite(&hrtc, ST_H, start_hour & 0x1F);
	HAL_RTCEx_BKUPWrite(&hrtc, ST_M, start_minutes & 0x3F);
	//HAL_PWR_DisableBkUpAccess();
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
		else if (buttons==7) {HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN); mode=0; currTime.Seconds=0; HAL_RTC_SetTime(&hrtc, &currTime, RTC_FORMAT_BIN);show_time();
			HAL_RTCEx_BKUPWrite(&hrtc, ST_H, 8U);
			HAL_RTCEx_BKUPWrite(&hrtc, ST_M, 0U);	
			} //trys mygtukai=00 sekundziu
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
					__disable_irq();
						if(buttons==2) {
											start_hour++;
											if (start_hour > 23) {tmp=0;}
											Write_Start_stop();
											mode_delay=MODE_DELAY;
										}
						else if(buttons==4) {
											start_hour--;
											if(start_hour > 25) {tmp=23;}
											Write_Start_stop();
											mode_delay=MODE_DELAY;
										}
						text[0]=start_hour/10+'0'; text[1]=start_hour % 10+'0';
						SSD1306_move(3,0); SSD1306_puts("Start hour:"); SSD1306_puts(text);
						__enable_irq();
					break;
					case 0x04: //setup start minute
					__disable_irq();
						if(buttons==2) {
											start_minutes++;
											if (start_minutes > 59) {start_minutes=0;}
											Write_Start_stop();
											mode_delay=MODE_DELAY;
										}
						else if(buttons==4) {
											start_minutes--;
											if(start_minutes > 60) {start_minutes=59;}
											Write_Start_stop();
											mode_delay=MODE_DELAY;
										}
						text[0]=start_minutes/10+'0'; text[1]=start_minutes % 10+'0';
						SSD1306_move(3,0); SSD1306_puts("Start min:"); SSD1306_puts(text);
						__enable_irq();
					break;
					case 0x05: //xxx
					break;
				}
			}
		}
  }
}



void show_time(void)
{
unsigned char font_dt[]={0x00, 0x00, 0xe7, 0xe7, 0xe7, 0xe7, 0x00, 0x00}; //dvitaskis

HAL_RTC_GetTime(&hrtc, &currTime, RTC_FORMAT_BIN);

SSD1306_bigdigit(0,1+0,currTime.Hours/10);
SSD1306_bigdigit(0,1+2,currTime.Hours%10);
SSD1306_bigdigit(0,1+5,currTime.Minutes/10);
SSD1306_bigdigit(0,1+7,currTime.Minutes%10);
SSD1306_bigdigit(0,1+10,currTime.Seconds/10);
SSD1306_bigdigit(0,1+12,currTime.Seconds%10);

SSD1306_move(1, 1+4);

SSD1306_put_tile(font_dt,8);

SSD1306_move(1, 1+9);
SSD1306_put_tile(font_dt,8);

//if(invertuotas>0){SSD1306_invert(); invertuotas--;} else {SSD1306_normal();}

}


// Programinis dugnas