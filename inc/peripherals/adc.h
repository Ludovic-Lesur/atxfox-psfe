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
#define ADC_ATX_VOLTAGE_CHANNEL		8
#define ADC_ATX_CURRENT_CHANNEL		0
// Bandgap voltage in mV.
#define ADC_BANDGAP_VOLTAGE_MV		2048
// ADC full scale result on 12-bits.
#define ADC_FULL_SCALE_12BITS		4095

/*** ADC functions ***/

void ADC1_Init(void);
void ADC_GetSupplyVoltageMv(unsigned int* supply_voltage_mv);
void ADC1_GetChannel12Bits(unsigned char channel, unsigned int* channel_result_12bits);

#endif /* ADC_H */
