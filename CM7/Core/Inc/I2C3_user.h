/**
 * I2C3_user.h
 *
 *  Created on: Jul 20, 2018
 *      Author: Torsten Jaekel
 */

#ifndef I2C3_USER_H_
#define I2C3_USER_H_

#include "stm32h7xx_hal.h"
#include <cmsis_os.h>
#include <semphr.h>
#include "main.h"
#include "VCP_UART.h"
#include "debug_sys.h"

#define USER_I2C_TIMEOUT		2000

extern I2C_HandleTypeDef hi2c3;
extern uint8_t I2CBuf[256];

int I2CUser_Setup(void);

int I2CUser_ClockSelect(int n);
int I2CUser_Read (uint16_t slaveAddr, uint8_t *pData, uint16_t num);
int I2CUser_Write(uint16_t slaveAddr, uint8_t *pData, uint16_t num);
int I2CUser_MemWrite(uint8_t *pData, uint16_t num);
int I2CUser_MemRead(uint16_t regAddr, uint8_t *pData, uint16_t num);
int I2CUser_MemWrite2(uint8_t *pData, uint16_t num);
int I2CUser_MemRead2(uint16_t regAddr, uint8_t *pData, uint16_t num);
int I2CUser_MemReadEx(uint16_t slaveAddr, uint16_t regAddr, uint8_t *pData, uint16_t num);

int PMIC_Recover(void);				//to external board, recover PMIC on other Portenta H7

/* for TOF sensor */
int32_t TOF_I2C_Init_Func(void);
int32_t TOF_I2C_DeInit_Func(void);
int32_t TOF_GetTick_Func(void);
int32_t TOF_I2C_WriteReg_Func(uint16_t, uint16_t, uint8_t *, uint16_t);
int32_t TOF_I2C_ReadReg_Func(uint16_t, uint16_t, uint8_t *, uint16_t);

#endif /* I2C3_USER_H_ */
