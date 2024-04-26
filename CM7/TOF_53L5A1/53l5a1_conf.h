/**
  ******************************************************************************
  * @file    53l5a1_conf.h
  * @author  IMG SW Application Team
  * @brief   This file contains definitions for the ToF components bus interfaces
  *          when using the X-NUCLEO-53L5A1 expansion board
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

#include "stm32h7xx_hal.h"
////#include "stm32l4xx_nucleo_bus.h"
#include "BSP_errno.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef VL53L5A1_CONF_H
#define VL53L5A1_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/*
 * the 53L5A1 BSP driver uses this symbol to allocate a structure for each device
 * if you are only using the on-board sensor without break-out boards
 * change its to (1U) in order to save space in RAM memory
 */
#define RANGING_SENSOR_INSTANCES_NBR    (3U)

#if 0
#define VL53L5A1_I2C_INIT               BSP_I2C1_Init
#define VL53L5A1_I2C_DEINIT             BSP_I2C1_DeInit
#define VL53L5A1_I2C_WRITEREG           BSP_I2C1_WriteReg16
#define VL53L5A1_I2C_READREG            BSP_I2C1_ReadReg16
#define VL53L5A1_GETTICK                BSP_GetTick
#else
#include "I2C3_user.h"
#define VL53L5A1_I2C_INIT               TOF_I2C_Init_Func
#define VL53L5A1_I2C_DEINIT             TOF_I2C_DeInit_Func
#define VL53L5A1_I2C_WRITEREG           TOF_I2C_WriteReg_Func
#define VL53L5A1_I2C_READREG            TOF_I2C_ReadReg_Func
#define VL53L5A1_GETTICK                TOF_GetTick_Func
#endif

#ifdef __cplusplus
}
#endif

#endif /* VL53L5A1_CONF_H*/
