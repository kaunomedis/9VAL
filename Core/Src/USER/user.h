/* file "user.h" */
/* ***********************************************
**                                              **
** 40mA   RTC clock V1.0 www.vabolis.lt (c)2023 **
**                                              **
************************************************ */

void user_usb_rx(uint8_t* Buf, uint32_t *Len);
void user_init(void);
void commandcom(char * txt);
void user_loop(void);
void user_seconds_job(void);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);


void show_time(void);

