/*
 * adc.h
 *
 *  Created on: 29 dec. 2018
 *      Author: Ludo
 */

#ifndef ADC_H
#define ADC_H

/*** ADC macros ***/

// ADC_INx channels.
#define ADC_BANDGAP_CHANNEL			9
#define ADC_VOLTAGE_SENSE_CHANNEL	8
#define ADC_CURRENT_SENSE_CHANNEL	0

/*** ADC functions ***/

void ADC1_Init(void);
void ADC1_GetChannel12Bits(unsigned char channel, unsigned int* channel_result_12bits);

#endif /* ADC_H */
