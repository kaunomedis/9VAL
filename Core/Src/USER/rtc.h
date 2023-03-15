/* ********************************************* */
/* (c)2022 by www.vabolis.lt, Kaunas             */
/**
  **********************************************
  * @file           : rtc.h
  * @version        : 1.0
  * @brief          : generic RTC clock and calendar for STM32
**/  
/* ********************************************* */


#ifndef RTC_H
#define RTC_H

void rtc_clean(void);
void rtc_update(void);
void rtc_set_time_text(char * Buf);
void rtc_set_date_text(char * Buf);

void rtc_int_init(void);
void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc);
void rtc_time_string(char * text);
void rtc_date_string(char * text);


void rtc_check_screwd_calendar(void);
void rtc_backup_date(void);
#endif
