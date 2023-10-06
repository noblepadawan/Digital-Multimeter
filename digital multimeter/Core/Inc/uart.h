/*
 * uart.h
 *
 *  Created on: May 4, 2023
 *      Author: kieran
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include "stm32l4xx_hal.h"

#define ESC 0x1B
#define lbracket 0x5B

void UART_init();
void UART_ESC_code(char *str);
void UART_print_string(char *str);
void UART_print_char(char c);
void uint16_to_string(uint16_t integer, char* str);


#endif /* INC_UART_H_ */
