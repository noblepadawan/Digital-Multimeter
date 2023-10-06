
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif


#define DC 0
#define AC 1

#define ADC_min_calib 20
#define ADC_max_calib 5

void SystemClock_Config(void);
void print_voltage(char*);
void print_graph(char*);

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
void Error_Handler(void);


#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
