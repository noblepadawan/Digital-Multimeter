
#include "main.h"
#include "arm_math.h"
#include "adc.h"
#include "uart.h"
#include "math.h"

// globals
float32_t samples[SAMPLES] = {0};
char SAMPLE_DONE = FALSE;
char mode = AC;
uint16_t voltage;
uint16_t vpp;
uint16_t dc_offset;


int main(void)
{
  HAL_Init();
  SystemClock_Config();
  ADC_init();
  UART_init();


  float32_t fft[SAMPLES];    // FFT data array
  uint16_t frequency;
  uint64_t max;
  uint64_t magnitude;

  // Character Printouts
  char cfreq[4];
  char cvolt[4];
  char cvolt2[4];
  char cvolt3[4];

  // Configure TIM2 for interrupt; ARR may change due to calibration
  RCC->APB1ENR1 |= (RCC_APB1ENR1_TIM2EN);
  TIM2->DIER |= (TIM_DIER_UIE);
  TIM2->ARR = 15750;
  TIM2->CR1 |= TIM_CR1_CEN;

  NVIC->ISER[0] = (1 << (TIM2_IRQn & 0x1F));
  __enable_irq();

  // instantiate fast Fourier transform struct
  arm_rfft_fast_instance_f32 fft_instance;
  arm_rfft_fast_init_f32(&fft_instance, SAMPLES);

  // Initialize table
  UART_ESC_code("2J");
  UART_ESC_code("8:0H");
  UART_print_string("|----|----|----|----|----|----|----");
  UART_ESC_code("9:0H");
  UART_print_string("0   0.5  1.0  1.5  2.0  2.5  3.0");


  while (1)
  {
    // wait for sample data to be collected and disable clock
	while(SAMPLE_DONE == FALSE);
	TIM2->CR1   		 &= ~(TIM_CR1_CEN);


	arm_rfft_fast_f32(&fft_instance, samples, fft, 0);

	// 0 is DC Offset
	// Find the max of the real, imaginary pairs
	// Ex: [2][3] = 1 Hz, [4][5] = 2 Hz

	max = SAMPLES;   	 // 1024 to prevent floating
	magnitude = 0;
    frequency = 0;

	for (uint16_t i = 2; i < SAMPLES; i += 2) {
   	   magnitude = (fft[i] * fft[i]) + (fft[i+1] * fft[i+1]);
   	   if (magnitude >= max){
   		 max = magnitude;
   		 frequency = i;// / 2;
   	   }
	}

	// Calibrate voltage
	ADC_volt_conv(voltage, cvolt);
	// Convert frequency to string
	uint16_to_string(frequency, cfreq);

	switch (mode)
	{
	case AC:
		ADC_volt_conv(vpp, cvolt2);					// calibrate peak-to-peak voltage
		ADC_volt_conv(dc_offset, cvolt3);			// calibrate DC offset
		UART_ESC_code("1:0H");   					// set cursor to line 1
		UART_ESC_code("2K");						// clear line
		UART_print_string("Mode: AC");				// print mode
		UART_ESC_code("2:0H");						// set cursor to line 2
		UART_ESC_code("2K");						// clear line
		UART_print_string("Frequency: ");			// print frequency
		UART_print_string(cfreq);
		UART_ESC_code("3:0H");						// set cursor to line 3
		UART_ESC_code("2K");						// clear line
		UART_print_string("Voltage (RMS): ");		// print Vrms
		print_voltage(cvolt);
		UART_ESC_code("4:0H");						// set cursor to line 4
		UART_ESC_code("2K");						// clear line
		UART_print_string("Voltage (VPP): ");		// print Vpp
		print_voltage(cvolt2);
		UART_ESC_code("5:0H");						// set cursor to line 4
		UART_ESC_code("2K");						// clear line
		UART_print_string("Voltage Offset: ");		// print Vpp
		print_voltage(cvolt3);
		break;
	case DC:
		UART_ESC_code("1:0H");   					// set cursor to line 1
		UART_ESC_code("2K");						// clear line
		UART_print_string("Mode: DC");				// print mode
		UART_ESC_code("2:0H");   					// set cursor to line 2
		UART_ESC_code("2K");						// clear line
		UART_print_string("Voltage (AVG): ");		// print Vavg
		print_voltage(cvolt);
		UART_ESC_code("3:0H");						// clear Vrms line
		UART_ESC_code("2K");
		UART_ESC_code("4:0H");						// clear Vpp line
		UART_ESC_code("2K");
		UART_ESC_code("5:0H");						// clear offset line
		UART_ESC_code("2K");
		break;
	}
	// print voltage to graph
	print_graph(cvolt);

	// reset globals
	voltage = 0;
	vpp = 0;
	dc_offset = 0;

	// wait
	for (int i = 0; i < 5000000; i++);
	SAMPLE_DONE = FALSE;
	TIM2->CR1   		 |= TIM_CR1_CEN;   		// re-enable clock
  }
}

void ADC1_2_IRQHandler(void) {
    if (ADC1 -> ISR & ADC_ISR_EOC) {
   	 //flag is reset by reading data
   	 static uint32_t i = 0;
   	 static uint32_t ADC_min = 0;
     static uint32_t ADC_max = 0;
     static uint32_t sum = 0;
     static float64_t sum2 = 0;
   	 if (i != SAMPLES){
   		 samples[i] = (float) ADC1 -> DR;
   		 if (samples[i] < ADC_min)		// check if less than current min
   		 {
   			 ADC_min = samples[i];
   		 }
   		 if (samples[i] > ADC_max)		// check if greater than current max
   		 {
   			 ADC_max = samples[i];
   		 }
   		 // add to sum for average
   		 sum += samples[i];

   		 // add to sum^2 for rms
   		 sum2 += (float64_t)(samples[i] * samples[i]);
   		 i++;
   	 }
   	 else {
   		 switch(mode)
   		 {
   		 case AC:
   			 // compute peak-to-peak voltage
   	   		 vpp = (ADC_max - ADC_max_calib) - (ADC_min + ADC_min_calib);
   	   		 // compute Vrms
   			 voltage = sqrt(sum2/SAMPLES);
   			 dc_offset = (ADC_max + ADC_min) / 2;
   			 break;
   		 case DC:
   			 voltage = sum / SAMPLES;;
   			 break;
   		 }
   		 i = 0;
   		 ADC_min = ADC_max;
   		 ADC_max = 0;
   		 sum = 0;
   		 sum2 = 0;
   		 SAMPLE_DONE = TRUE;
   	 }
    }
}


void TIM2_IRQHandler(void)
{
  // check status register for update event flag
    if (TIM2->SR & TIM_SR_UIF) {
   	   ADC1 -> CR |= ADC_CR_ADSTART;
   	   TIM2 -> SR &= ~(TIM_SR_UIF);
    }
}

void USART2_IRQHandler(void)
{
	// Check if character has been received
	if (USART2->ISR & USART_ISR_RXNE)
	{
		uint8_t received = USART2->RDR;

		switch(received)
		{
			case ('A' | 'a'):
				mode = AC;	// set mode to AC
				break;
			case ('D' | 'd'):
				mode = DC;	// set mode to DC
				break;
		}
	}
	// Reset received data flag
	USART2->ISR &= ~(USART_ISR_RXNE);

}

void print_graph(char* volt)
{
	UART_ESC_code("7:0H");
	UART_ESC_code("2K");		// clear line
	uint16_t i;
	uint16_t intvalue = 0;

	// convert char array back to int
	for (i = 0; i < 4; i++) {
		intvalue = intvalue * 10 + (volt[i] - '0');
	}
	// print to graph
	for (i = 0; i <= intvalue; i+=100){
		UART_print_char('X');
	}
	UART_ESC_code("10:0H");
}

void print_voltage(char* volt)
{
	char c = *volt;
	UART_print_char(c);				// print first digit
	UART_print_char(0x2e);			// add decimal
	c = *(volt+1);					// shift to first decimal
	UART_print_char(c);				// print next digit
	c = *(volt+2);					// shift to second decimal
	UART_print_char(c);				// print next digit
	c = *(volt+3);					// shift to third decimal
	UART_print_char(c);				// print next digit
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
