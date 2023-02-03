/**
 * ITM_print_log.h
 *
 *  Created on: Jun 5, 2018
 *      Author: Torsten Jaekel
 */

#ifndef ITM_PRINT_LOG_H_
#define ITM_PRINT_LOG_H_

/* potentially already included via arduino.h or mbed.h */
////#include "stm32h745xx.h"		//needed for IRQn_Type

/**
* ATTENTION: for SWO we need PB3:
* - but PB3 conflicts with SD Card, (SDC_D2)
* - you lose SD Card: and if SD Card is initalized - do the
*   ITM_enable afterwards
*/

#include "stm32h7xx_hal.h"		//for GPIO_PIN_3
#include "system_stm32h7xx.h"	//for SystemCoreClock
#include "core_cm7.h"

void ITM_enable(void);
void ITM_print(unsigned char *str);

#endif /* ITM_PRINT_LOG_H_ */
