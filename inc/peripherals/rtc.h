/*
 * rtc.h
 *
 *  Created on: 18 apr. 2020
 *      Author: Ludo
 */

#ifndef RTC_H
#define RTC_H

/*** RTC functions ***/

void RTC_Reset(void);
void RTC_Init(unsigned int lsi_freq_hz);
void RTC_StartWakeUpTimer(unsigned int delay_seconds);
volatile unsigned char RTC_GetWakeUpTimerFlag(void);
void RTC_ClearWakeUpTimerFlag(void);

#endif /* RTC_H */
