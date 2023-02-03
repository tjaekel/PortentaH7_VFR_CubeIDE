/*
 * picoc_cmd_if.c
 *
 *  Created on: Aug 19, 2022
 *      Author: tj
 */
#include "VCP_UART.h"
#include "cmd_dec.h"
#include "GPIO_user.h"
#include "SPI.h"
#include "I2C3_user.h"
#include "picoc.h"
#include <cmsis_os2.h>

static int sPicoC_Stopped = 0;

void picoc_ClearStopped(void)
{
	sPicoC_Stopped = 0;
}

void picoc_SetStopped(void)
{
	sPicoC_Stopped = 1;
}

int picoc_CheckForStopped(void)
{
    return sPicoC_Stopped;
}

int picoc_CheckForAbort(void)
{
	/* read UART not-waiting and check for CTRL-C */
	unsigned char c[1];
	if (CDC_GetRx(c, 1))
	{
		if (c[0] == 0x03)
		{
			GpicocAbort = 1;
			longjmp(ExitBuf, 1);
			return 1;
		}
	}
    return 0;
}

void picoc_MsSleep(unsigned long ms)
{
	osDelay(ms);
}

int picoc_ExecuteCommand(char *s)
{
	extern ECMD_DEC_Status CMD_DEC_execute(char *cmd, EResultOut out);
	CMD_DEC_execute(s, UART_OUT);
    return 0;
}

int picoc_SpiTransaction(unsigned char *tx, unsigned char *rx, int bytes)
{
	return SPI_TransmitReceive(tx, rx, bytes);
}

int picoc_I2CRead(unsigned char slaveAddr, unsigned char regAddr, unsigned char *data, int bytes, int flags)
{
	return I2CUser_MemReadEx(slaveAddr, regAddr, data, (uint16_t)bytes);
}

int picoc_I2CWrite(unsigned char slaveAddr, unsigned char *data, int bytes, int flags)
{
	return I2CUser_Write((uint16_t)slaveAddr, data, (uint16_t)bytes);
}

void picoc_WriteGPIO(unsigned long val)
{
	GPIO_Set(val, 0xFFFFFFFF);
}

unsigned long picoc_ReadGPIO(void)
{
	return GPIO_Get();
}

