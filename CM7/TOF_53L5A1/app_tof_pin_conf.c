/**
  ******************************************************************************
  * @file    app_tof_pin_conf.c
  * @author  IMG SW Application Team
  * @brief   This file contains functions for TOF pins
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#define STM32L4xx

/* Includes ------------------------------------------------------------------*/
#include "app_tof_pin_conf.h"

extern volatile uint8_t ToF_EventDetected;

#ifdef STM32G0xx
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == TOF_INT_EXTI_PIN)
  {
    ToF_EventDetected = 1;
  }
}
#else
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
#if 0
  if (GPIO_Pin == TOF_INT_EXTI_PIN)
  {
    ToF_EventDetected = 1;
  }
#endif
}
#endif
