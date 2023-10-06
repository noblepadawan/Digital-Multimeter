/*
 * adc.c
 *
 *  Created on: May 19, 2023
 *      Author: kieran
 */

#include "adc.h"

void ADC_init()
{
	// Configure PA0 for ADC
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER &= ~(GPIO_MODER_MODE0);
	GPIOA->MODER |= (GPIO_MODER_MODE0);
	GPIOA->ASCR |= (GPIO_ASCR_ASC0);		// connect PA0 analog switch to ADC input

	//enable ADC Clock
	RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
	// ADC will run at same speed as CPU
	ADC123_COMMON->CCR = (1 << ADC_CCR_CKMODE_Pos);

	//power up ADC and turn on voltage regulator
	ADC1->CR &= ~(ADC_CR_DEEPPWD);
	ADC1->CR |= (ADC_CR_ADVREGEN);
	for (uint32_t i = 0; i < 640; i++); 	// wait 20 microseconds for regulator to power up
											// 4 Mhz -> 80
											// 32 MHZ -> 640
											// 80 Mhz -> 1600
	//calibrate ADC
	//ensure ADC is not enabled, single ended calibration
	ADC1->CR &= ~(ADC_CR_ADEN | ADC_CR_ADCALDIF);
	ADC1->CR |= (ADC_CR_ADCAL);				// start calibration
	while(ADC1->CR & ADC_CR_ADCAL);			// wait for calibration to finish

	//configure single ended mode for channel 5 before enabling ADC
	ADC1->DIFSEL &= ~(ADC_DIFSEL_DIFSEL_5);

	//enable ADC
	ADC1->ISR |= (ADC_ISR_ADRDY);			// clear ready flag with a 1
	ADC1->CR |= (ADC_CR_ADEN); 				// enable ADC
	while (!(ADC1->ISR & ADC_ISR_ADRDY));	// wait for ADC ready flag
	ADC1->ISR |= (ADC_ISR_ADRDY);			// clear ready flag with a 1

	//configure ADC
	//set sequence to 1 conversion on channel 5
	ADC1->SQR1 = (5 << ADC_SQR1_SQ1_Pos);

	//configure sampling time of 2.5 clock cycles for channel 5
	ADC1->SMPR1 &= ~(ADC_SMPR1_SMP5);

	//ADC configuration 12-bit software trigger
	// right align
	ADC1->CFGR = 0;

	// start conversion
	ADC1->CR |= (ADC_CR_ADSTART);

	//enable interrupts for ADC
	ADC1->IER |= (ADC_IER_EOCIE);			// interrupt on end of conversion
	ADC1->ISR &= ~(ADC_ISR_EOC);			// clear EOC flag

	NVIC->ISER[0] = (1 << (ADC1_2_IRQn & 0x1F));	// enable interrupt in NVIC
}


void ADC_volt_conv(uint16_t dig_mvolt, char* retval)
// Calibrate millivolts and convert to a string for UART
{
	uint16_t mvolt;
	int64_t calib = (VREF * dig_mvolt) / RES;
	int64_t uvolt = CALIBRATED_MULT * calib - CALIBRATED_OFFSET;
	if (uvolt < 0) {mvolt = calib;}
	else {mvolt = uvolt / 1000;}

	uint8_t index = 0;

	while(index < 4 || mvolt != 0)	// make sure value is 4 digits
	{
		uint16_t digit = mvolt % 10;	// isolate digit
		retval[index] = digit + '0';	// '0' converts to ASCII character
		mvolt /= 10;
		index++;
	}

    // Reverse the string
    uint8_t i = 0, j = index - 1;
    while (i < j)
	{
        char temp = retval[i];
        retval[i] = retval[j];
        retval[j] = temp;
        i++;
        j--;
	}
}
