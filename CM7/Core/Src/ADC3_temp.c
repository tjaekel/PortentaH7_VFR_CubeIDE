/*
 * ADC3_temp.c
 *
 *  Created on: Nov 22, 2022
 *      Author: tj
 */

#include "ADC3_temp.h"

ADC_HandleTypeDef hadc3;

uint16_t ADC_Val[5];
uint32_t VRefInt, VBat, TSensor, ADC_INP0, ADC_INP1;

void ADC3_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  /** Common config
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;		//running with PLL2
  hadc3.Init.Resolution = ADC_RESOLUTION_16B;
  hadc3.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc3.Init.LowPowerAutoWait = DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.NbrOfConversion = 5;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc3.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc3.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc3.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    //Error_Handler();
  }

  /** Configure Regular Channel */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;	//ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    //Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_VBAT;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
      //Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    //Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    //Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    //Error_Handler();
  }

  HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
}

/**
* @brief ADC MSP Initialization
* This function configures the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  if(hadc->Instance == ADC3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_ADC3_CLK_ENABLE();
  }
}

/**
* @brief ADC MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
  if(hadc->Instance == ADC3)
  {
	/* Peripheral clock disable */
    __HAL_RCC_ADC3_CLK_DISABLE();
  }
}

/* ----------------------------------------------------------------- */

int ADC3_RunOnce(EResultOut out, int flag)
{
	HAL_ADC_Start(&hadc3);
	for (uint8_t i = 0; i < (sizeof(ADC_Val) / sizeof(ADC_Val[0])); i++)
	{
		HAL_ADC_PollForConversion(&hadc3, 1000);
		ADC_Val[i] = HAL_ADC_GetValue(&hadc3);
	}
	HAL_ADC_Stop(&hadc3);

	VRefInt 	= __HAL_ADC_CALC_VREFANALOG_VOLTAGE(ADC_Val[0], hadc3.Init.Resolution);
	VBat 		= __HAL_ADC_CALC_DATA_TO_VOLTAGE(VRefInt, ADC_Val[1], hadc3.Init.Resolution);
	TSensor	    = __HAL_ADC_CALC_TEMPERATURE(VRefInt, ADC_Val[2], hadc3.Init.Resolution);
	ADC_INP0 	= __HAL_ADC_CALC_DATA_TO_VOLTAGE(VRefInt, ADC_Val[3], hadc3.Init.Resolution);
	ADC_INP1 	= __HAL_ADC_CALC_DATA_TO_VOLTAGE(VRefInt, ADC_Val[4], hadc3.Init.Resolution);

	//VBat value is 1/4 when read
	VBat *= 4;

	/* we have to compensate the temperature, formula:
	 * Temp (in C) = (100 - 30) / (TS_CAL2 - TS_CAL1) x (TS_DATA = TS_CAL1) + 30
	 * This is done by macro __HAL_ADC_CALC_TEMPERATURE
	 */

	if (flag)
		print_log(out, "MCU params  : VRef: %ld.%ldV VBat: %ld.%ldV Temp: %ldC\r\n", VRefInt/1000, VRefInt%1000, VBat/1000, VBat%1000, TSensor);

	return 1;
}

int ADC3_Voltage(EResultOut out)
{
	/* WHY a voltage on ADC_INP0 does show a voltage on ADC_INP1? */
	print_log(out, "INP0: %ld.%ldV | INP1: %ld.%ldV\r\n", ADC_INP0/1000, ADC_INP0%1000, ADC_INP1/1000, ADC_INP1%1000);

	return 1;
}
