#ifndef INC_ADC_H_
#define INC_ADC_H_

#include "stm32l4xx_hal.h"

void ADC_init(void);
void ADC_volt_conv(uint16_t, char*);

#define TRUE 1
#define FALSE 0

#define SAMPLES 1024

#define VREF 3300
#define RES 4096

#define CALIBRATED_MULT 1014
#define CALIBRATED_OFFSET 6684

#endif /* INC_ADC_H_ */
