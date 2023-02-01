/*
 * ADC3_temp.h
 *
 *  Created on: Nov 22, 2022
 *      Author: tj
 */

#ifndef INC_ADC3_TEMP_H_
#define INC_ADC3_TEMP_H_

#include "stm32h7xx_hal.h"
#include "main.h"
#include "VCP_UART.h"
#include "debug_sys.h"

extern void ADC3_Init(void);
extern int ADC3_RunOnce(EResultOut out, int flag);
extern int ADC3_Voltage(EResultOut out);

#endif /* INC_ADC3_TEMP_H_ */
