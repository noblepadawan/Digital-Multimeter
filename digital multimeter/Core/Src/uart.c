/*
 * uart.c
 *
 *  Created on: May 4, 2023
 *      Author: kieran
 */

#include "uart.h"

#define SYS_CLK 4000000
#define BAUD 115200


void UART_init()
{
	//Enable USART timer
	RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

	//Enable GPIO timer
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
	GPIOA->MODER |= (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
	//GPIOA->MODER &= ~(GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);	// push pull output
	GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL2 | GPIO_AFRL_AFRL3);
	GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos);			// Enable TX on PA2
	GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL3_Pos);			// Enable RX on PA3


	USART2->CR1 &= ~(USART_CR1_UE);							// Turn off USART
	USART2->CR1 |= (USART_CR1_RE | USART_CR1_TE);			// Enable transmitter and receiver
	USART2->CR1 &= ~(USART_CR1_OVER8);						// Set to oversample by 16
	USART2->BRR = 277;										// 32MHz / 115.2kbps ~ 277
	USART2->CR2 &= ~(USART_CR2_STOP);						// Set 1 bit stop
	USART2->CR1 &= ~(USART_CR1_M1 | USART_CR1_M0);			// Set to 8 data bits
	USART2->CR1 &= ~(USART_CR1_PCE);						// Set no parity
	USART2->CR1 |= (USART_CR1_UE);							// Enable USART


    // Enable interrupt in NVIC
	USART2->CR1 |= USART_CR1_RXNEIE;
    NVIC->ISER[1] = (1 << (USART2_IRQn & 0x1F));


}

void UART_print_string(char *str)
{
    while (*str != '\0') {
        // Wait for the TX buffer to be empty
        while (!(USART2->ISR & USART_ISR_TXE));

        // Send the character
        USART2->TDR = (*str);

        // Wait for transmission complete
        while (!(USART2->ISR & USART_ISR_TC));

        // Move to the next character
        str++;

        // Add a delay to ensure previous transmission is complete
        for (int i = 0; i < 1000; i++);
    }
}

void UART_print_char(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE));

    // Send the character
    USART2->TDR = c;

    // Wait for transmission complete
    while (!(USART2->ISR & USART_ISR_TC));

    // Add a delay to ensure previous transmission is complete
    for (int i = 0; i < 1000; i++);
}

void UART_ESC_code(char *str)
{
		UART_print_char(ESC); // ESC character
		UART_print_char(lbracket); // [ character
		UART_print_string(str);
}


void uint16_to_string(uint16_t integer, char* str)
{
    uint16_t divisor = 10000;
    int index = 0;
   	 // Handle the case where the value is zero
   	 if (integer == 0) {
   		 str[index++] = '0';
   		 str[index] = '\0';
   		 return;
    }
   	 // Handle the case where the value is greater than zero
   	 while (divisor > 0) {
   		 uint16_t quotient = integer / divisor;
   		 if (quotient > 0 || index > 0) {
   			 str[index++] = quotient + '0';
   			 integer -= quotient * divisor;
    }
   		 divisor /= 10;
    }
    str[index] = '\0';
}
