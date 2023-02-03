/*
 * power_mngt.h
 *
 *  Created on: Feb 1, 2023
 *      Author: tj
 */

#ifndef POWER_MNGT_H_
#define POWER_MNGT_H_

#include "main.h"
#include "I2C_PMIC.h"
#include "stm32h7xx_ll_pwr.h"
#include "stm32h7xx_ll_cortex.h"

#define PWR_UNDEF		0
#define PWR_FULL		(1 << 0)
#define PWR_DVS			(1 << 1)
#define PWR_SLEEP		(1 << 2)

extern int SetSysClock_PLL_HSE(uint8_t bypass, int lowspeed);

int PWR_Set(int mode);
int PWR_GetMode();

#endif /* POWER_MNGT_H_ */
