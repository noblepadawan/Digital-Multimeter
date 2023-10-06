
#include "main.h"
#include "arm_math.h"
#include "adc.h"
#include "uart.h"

void SystemClock_Config(void);

// globals
float32_t samples[SAMPLES];
uint16_t SAMPLE_DONE = FALSE;

/**
  * @brief  The application entry point.
  * @retval int
  */
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
  char s[4];

  // Configure TIM2 for interrupt; ARR may change due to calibration
  RCC -> APB1ENR1    	 |= (RCC_APB1ENR1_TIM2EN);
  TIM2-> DIER   		 |= (TIM_DIER_UIE);
  TIM2-> ARR   		  = 15725;
  TIM2->CR1   	        |= TIM_CR1_CEN;

  NVIC->ISER[0] = (1 << (TIM2_IRQn & 0x1F));
  __enable_irq();

  // instantiate fast Fourier transform struct
  arm_rfft_fast_instance_f32 fft_instance;
  arm_rfft_fast_init_f32(&fft_instance, SAMPLES);

  while (1)
  {
    // wait for sample data to be collected and disable clock
	while(SAMPLE_DONE == FALSE);
	TIM2->CR1   		 &= ~(TIM_CR1_CEN);

	arm_rfft_fast_f32(&fft_instance, samples, fft, 0);

	// 0 is DC Offset
	// Find the max of the real, imaginary pairs
	// Ex: [2][3] = 1 Hz, [4][5] = 2 Hz

	max = SAMPLES;   	 // 2048 to prevent floating
	magnitude = 0;
       frequency = 0;

	for (uint16_t i = 2; i < SAMPLES; i += 2) {
   	   magnitude = (fft[i] * fft[i]) + (fft[i+1] * fft[i+1]);
   	   if (magnitude >= max){
   		 max = magnitude;
   		 frequency = i / 2;
   	   }
	}

	uint16_to_string(frequency, s);

       UART_ESC_Code("[2J");   			       // clear screen
       UART_ESC_Code("[H");   				// set cursor to top left
	UART_print(s);   					// print
	for (int i = 0; i < 100000; i++);
	SAMPLE_DONE = FALSE;
	TIM2->CR1   		 |= TIM_CR1_CEN;   		// re-enable clock
  }
}

/* ADC IRQ:
 * retrieves the data from the ADC on interrupt
 */
void ADC1_2_IRQHandler(void) {
    if (ADC1 -> ISR & ADC_ISR_EOC) {
   	 //flag is reset by reading data
   	 static uint32_t i = 0;
   	 if (i != SAMPLES){
   		 samples[i] = (float32_t) ADC1 -> DR;
   		 i++;
   	 }
   	 else {
   		 SAMPLE_DONE = TRUE;
   		 i = 0;
   	 }
    }
}
// Timer 2 ISR
void TIM2_IRQHandler(void)
{
  // check status register for update event flag
    if (TIM2->SR & TIM_SR_UIF) {
   	   ADC1 -> CR |= ADC_CR_ADSTART;
   	   TIM2 -> SR &= ~(TIM_SR_UIF);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
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

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
