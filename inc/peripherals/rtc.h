/*
 * rtc.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef RTC_H
#define RTC_H

/*** RTC functions ***/

void RTC_reset(void);
void RTC_init(unsigned int lsi_freq_hz);
void RTC_start_wakeup_timer(unsigned int delay_seconds);
volatile unsigned char RTC_get_wakeup_timer_flag(void);
void RTC_clear_wakeup_timer_flag(void);

#endif /* RTC_H */
