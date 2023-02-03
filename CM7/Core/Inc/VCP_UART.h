/**
 * VCP_UART.h
 *
 *  Created on: Jul 18, 2022
 *      Author: Torsten Jaekel
 */

#ifndef INC_VCP_UART_H_
#define INC_VCP_UART_H_

#include <stdint.h>
#include <stdio.h>

#include "usbd_cdc_if.h"
#include "ITM_print_log.h"

#define UART_RX_BUFFER_SIZE	4096	//1024
#define UART_TX_BUFFER_SIZE 4096	//1024

typedef enum {
	UART_OUT,
	HTTPD_OUT,
	ITM_OUT,
	SILENT
} EResultOut;

extern unsigned char uartPrintBuffer[UART_RX_BUFFER_SIZE];

#define print_log(out, ...)			do {\
									snprintf((char *)uartPrintBuffer, UART_TX_BUFFER_SIZE, __VA_ARGS__);\
									UART_printString(uartPrintBuffer, out);\
								} while (0)
int UART_getChar(void);
int UART_WaitForChar(void);
void UART_putChar(unsigned char c);
int UART_getString(unsigned char *buf, int maxLen);
void UART_Send(unsigned char *buf, int len, EResultOut out);
void UART_printString(unsigned char *str, EResultOut out);
void UART_setLastString(unsigned char *last);

#endif /* INC_VCP_UART_H_ */
