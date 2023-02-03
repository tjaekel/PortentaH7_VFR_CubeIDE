/*
 * PWM_servo.c
 *
 *  Created on: Jan 13, 2023
 *      Author: tj
 */

/*
 * add to system_clock.s:
 * PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_SPI2 | RCC_PERIPHCLK_QSPI | RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_SAI4A | RCC_PERIPHCLK_TIM;
   PeriphClkInitStruct.Lptim345ClockSelection = RCC_LPTIM345CLKSOURCE_PCLK4;

   We do not need a TIM INT here.
 */

#include "PWM_servo.h"

TIM_HandleTypeDef htim3;

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	//initialize GPIO
	if (htim == &htim3)
	{
		__HAL_RCC_GPIOC_CLK_ENABLE();

		//PC6 : PWM TIM3 CH1
		GPIO_InitStruct.Pin = GPIO_PIN_6;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		//PC7 : PWM TIM3 CH2
		GPIO_InitStruct.Pin = GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

#if 0
		//we do not need an INT for PWM TIM
		HAL_NVIC_SetPriority(TIM3_IRQn, 0x0F, 0);
		HAL_NVIC_EnableIRQ(TIM3_IRQn);
#endif
	}
}

void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;

  __HAL_RCC_TIM3_CLK_ENABLE();

  /* we use RCC_LPTIM345CLKSOURCE_PCLK4: obviously 49 MHz */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 490-1;			//get 500 KHz clock
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 10000;			//get 50 Hz: we get 49.996 Hz = 20 ms
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(&htim3);

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig);

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 750;			//neutral, 1.5 ms
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
  HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);
}

void user_pwm_setvalue(int num, uint16_t value)
{
    TIM_OC_InitTypeDef sConfigOC;

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = value;		//period is 20,000 micro-sec., so value is ratio, e.g. 100 = 200 micro-sec
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if ( ! num)
    {
    	HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1);
    	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    }
    else
    {
    	HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2);
    	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    }
}

uint16_t servoVal[2] = {500, 1000};	//500: = 0 deg, 750 = 90 deg, 1000 = 180 deg.

void PWM_ServoInit(void)
{
	MX_TIM3_Init();

	//the ratio: 20,000 micro-sec, e.g. 1000 = 2,000 micro-sec = 2 ms
	//for servos the pulse period is in range: 1 ms .. 2 ms = 500 .. 1000, 750 is neutral
	user_pwm_setvalue(0, servoVal[0]);
	user_pwm_setvalue(1, servoVal[1]);
}

void PWM_ServoSet(int num, int degree)
{
	/*
	 * why we cannot update just the .pulse? It stops the PWM!
	 * we have to do all again, including the basic config
	 * Remark: some servos work with 3V3, but some not: they rotate several times:
	 * So,
	 * power the servo from 5V, not the 3V3 VDD on board: it helps also to overload the PMIC chip
	 */

	float val;

	//set servo in range 0..180 degree
	if ((degree < 0) || (degree > 180))
		return;

	//we have 500 as value range, covering 180 degree
	val = 500.0f + ((float)degree * 500.0f) / 180.0f;

	if ( ! num)
		servoVal[0] = (uint16_t)val;
	else
		servoVal[1] = (uint16_t)val;

	/* do all again to configure - why does just this work? */
	PWM_ServoInit();
}
