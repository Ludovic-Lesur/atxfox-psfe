/*
 * lptim.h
 *
 *  Created on: 9 juil. 2019
 *      Author: Ludo
 */

#ifndef LPTIM_H
#define LPTIM_H

/*** LPTIM functions ***/

void LPTIM1_init(unsigned int lsi_freq_hz);
void LPTIM1_delay_milliseconds(unsigned int delay_ms);

#endif /* PERIPHERALS_LPTIM_H_ */
