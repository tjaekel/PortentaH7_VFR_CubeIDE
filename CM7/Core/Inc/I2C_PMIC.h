/**
 * I2C_PMIC.h
 *
 *  Created on: Jul 10, 2022
 *      Author: tjaekel
 */

#ifndef I2C_PMIC_H_
#define I2C_PMIC_H_

#include "stm32h7xx_hal.h"

#define I2C_TIMEOUT		2000

extern I2C_HandleTypeDef hi2cPMIC;

int I2C_PMIC_Setup(void);
int I2C_PMIC_Read (uint16_t slaveAddr, uint8_t *pData, uint16_t num);	//not used: not to read registers
int I2C_PMIC_Write(uint16_t slaveAddr, uint8_t *pData, uint16_t num);

/* slave address PMIC */
#define PMIC_I2C_SLAVE_ADDR	(0x08 << 1)

int I2C_PMIC_Initialize(void);
void I2C_PMIC_LowVoltage(void);
void I2C_PMIC_HighVoltage(void);

/* register read command */
int I2C_PMIC_MemRead(uint16_t slaveAddr, uint16_t regAddr, uint8_t *pData, uint16_t num);
int I2C_PMIC_Write(uint16_t slaveAddr, uint8_t *pData, uint16_t num);	//also as WriteReg

#endif /* I2C_PMIC_H_ */
