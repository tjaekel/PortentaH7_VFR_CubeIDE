/**
 * VCP_UART.c
 *
 *  Created on: Jul 18, 2022
 *      Author: Torsten Jaekel
 */

#include "usbd_cdc_if.h"
#include "string.h"
#include "cmsis_os.h"
#include "VCP_UART.h"
#include "httpserver_netconn.h"

static unsigned char uartRxStrBuf[UART_RX_BUFFER_SIZE];
unsigned char uartPrintBuffer[UART_RX_BUFFER_SIZE];				//global, for print_log() in all other files as macro

static int lastUARTchar;
static unsigned char lastUARTStr[UART_RX_BUFFER_SIZE];
static unsigned int idx = 0;

int UART_getChar(void)
{
	/* this function waits until something received
	 * due to RTOS used, but we do not have a semaphore from USB VCP - osDelay() is used
	 * TODO: add a semaphore from USB CDC instead of polling all the time
	 */
	unsigned char rx;
	int l;
	while (1)
	{
		l = CDC_GetRx(&rx, 1);
		if (l)
			return (int)rx;
		else
		{
			osThreadYield();
			osDelay(5);	//XXXX
			/* ATT: copy and paste in UART terminal or sending a file (both larger) - CRASHES! */
		}
	}
}

int UART_WaitForChar(void)
{
	unsigned char rx;

	if (CDC_GetRx(&rx, 1))
		return 1;
	else
		return 0;
}

/**
 * TODO: we need a function to wait for UART is calm, e.g. after CTRL-C,
 * otherwise we get the CTRL-C into next command buffer,
 * needed for Pico-C to check for CTRL-C as abort (or kill),
 * for now: the CTRL-C must be hold/pressed very shortly!
 */

void UART_putChar(unsigned char c)
{
	unsigned char cr = '\r';
	if (c == '\n')
		CDC_PutTx(&cr, 1);
	CDC_PutTx(&c, 1);
}

int UART_getString(unsigned char *buf, int len)
{
		/**
		 * TODO: ATTENTION: function does not check buffer overflows!!!
		 */

		static int ignoreNext = 0;
		int ch;
		unsigned int l;

		if ((ch = UART_getChar()) != -1)
		{
			lastUARTchar = ch;

			/*
			 * regular ASCII mode, with ESC handling (UP and DONW key)
			 */
			//following ESC sequence characters - ignore it
			if (ignoreNext)
			{
				ignoreNext--;
				if (ignoreNext == 0)
				{
					if (ch == 'A')
					{
						//arrow up
						if (idx == 0)
						{
							idx = (int)strlen((char *)lastUARTStr);
							if (idx)
							{
								CDC_PutTx((uint8_t *)lastUARTStr, idx);
								memcpy(uartRxStrBuf, lastUARTStr, idx);
							}
							else
								uartRxStrBuf[0] = '\0';
						}
					}
					if (ch == 'B')
					{
						//arrow down
						;
					}
				}
				return 0;
			}

			//handle backspace
			if (ch == 0x08)
			{
				if (idx > 0)
				{
					idx--;
					CDC_PutTx((uint8_t *)"\010 \010", 3);
				}
				else
				{
					;
				}

				return 0;
			}

			//handle TAB: convert into spaces
			if (ch == '\t')
			{
				uartRxStrBuf[idx +0] = (uint8_t)' ';
				uartRxStrBuf[idx +1] = (uint8_t)' ';
				uartRxStrBuf[idx +2] = (uint8_t)' ';
				uartRxStrBuf[idx +3] = (uint8_t)' ';
				CDC_PutTx(&uartRxStrBuf[idx], 4);
				idx += 4;

				return 0;
			}

			//ignore ESC sequences - do not echo, assume just few escape sequences here
			if (ch == 0x1B)
			{
				ignoreNext = 2;			//assuming just arrows up, down etc.
										//as: 1B [ A  (or 1B [ B etc.)
				return 0;
			}

			//ignore '\0'
			if (ch == '\0')
				return 0;

			uartRxStrBuf[idx] = (uint8_t)ch;

			//local echo - translate Carriage Return into Newline + Carriage Return
			if (uartRxStrBuf[idx] == '\n')
			{
				CDC_PutTx((uint8_t *)"\r", 1);	/* keep always 0x0D 0x0A */
				CDC_PutTx(&uartRxStrBuf[idx], 1);
			}
			else if (uartRxStrBuf[idx] == '\r')
			{
				CDC_PutTx(&uartRxStrBuf[idx], 1);	/* keep always 0x0D 0x0A */
				CDC_PutTx((uint8_t *)"\n", 1);
			}
			else
				CDC_PutTx(&uartRxStrBuf[idx], 1);

			idx++;
			if (idx >= (len - 1))
			{
				memcpy(buf, uartRxStrBuf, (size_t)(len - 1));
				*(buf + len - 1) = '\0';
				idx = 0;
				strncpy((char *)lastUARTStr, (char *)buf, sizeof(lastUARTStr) - 1);
				return len - 1;
			}

			if ((uartRxStrBuf[idx - 1] == '\r') || (uartRxStrBuf[idx - 1] == '\n'))
			{
				memcpy(buf, uartRxStrBuf, (size_t)idx);
				*(buf + idx) = '\0';
				*(buf + idx + 1) = '\0';		//make sure not to print old message via 'printENTER'
				l = idx;
				if (l > 1)
				{
					strncpy((char *)lastUARTStr, (char *)buf, sizeof(lastUARTStr) - 1);
					lastUARTStr[idx - 1] = '\0';
				}
				idx = 0;
				return l;
			}
		}

		return 0;
}

void __attribute__((section(".itcmram"))) UART_Send(unsigned char *buf, int len, EResultOut out)
{
	if ((out == UART_OUT) || ((GDebug & DBG_LOG_BOTH) && (out != ITM_OUT)))
		CDC_PutTx(buf, (uint16_t)len);
	if (out == HTTPD_OUT)
		HTTPD_PrintOut(buf, len);
	if (out == ITM_OUT)
		ITM_print(buf);
}

void __attribute__((section(".itcmram"))) UART_printString(unsigned char *str, EResultOut out)
{
	size_t len;
	len = strlen((char *)str);
	UART_Send(str, (int)len, out);
}

void UART_setLastString(unsigned char *last)
{
	/* remove the last '\r' - we print with ESC arrow up - cursor at the end */
	int l;
	l = strlen((char *)last);
	if (l)
		if (*(last + l - 1) == '\r')
			*(last + l - 1) = '\0';

	if (strlen((char *)last))
		strcpy((char *)lastUARTStr, (const char *)last);
}
