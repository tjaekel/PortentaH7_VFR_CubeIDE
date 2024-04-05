/**
 * main.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h747xx.h"
#include "stm32h7xx_hal.h"

#include "debug_sys.h"
#include "syserr.h"
#include "VCP_UART.h"

#define ENABLE_DCACHE		/* no real difference - MPU disables all the caches? */

/**
 * macro for Version Info Welcome message
 */
#define STR_CHIP			"STM32H747"
#define STR_FW_VERSION		"V2.00"

#define PRINT_VERSION		UART_Send((uint8_t *)"\r\n\r\n**** Portenta H7 VFR " STR_FW_VERSION " ****\r\n-------------------------------\r\n", 70, UART_OUT)
#define PROJECT_NAME		"Portenta H7 VFR"

////void Error_Handler(void);

#define	UART_STR_SIZE		4096
extern unsigned char strRxBuf[UART_STR_SIZE];

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
