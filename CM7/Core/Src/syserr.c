/** syserr.c
 *
 *  Created on: Oct 23, 2019
 *      Author: Torsten Jaekel
 */

#include "syserr.h"
#include "stdio.h"
#include "main.h"

extern char GitmBuf[120];

unsigned long GSysErr;

void /*__attribute__((section(".itcmram")))*/ SYS_SetError(unsigned long errCode)
{
	/* turn red LED on if error set */
			//red LED On
	GSysErr |= errCode;

	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_RESET);		//red LED on

	//always!!
	////sprintf(GitmBuf, "*E: SYSERR: %08lX\r\n", SYS_GetError());
	////ITM_print(GitmBuf);

	//with Debug Verbose: print every SYS_SetError
}

inline unsigned long SYS_GetError(void)
{
	return GSysErr;
}

void SYS_ClrError(void)
{
	//red LED Off
	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_SET);		//red LED off
	GSysErr = 0;
}

void SYS_DisplayError(EResultOut out)
{
	unsigned long err;
	unsigned long mask = 1;
	int i;
	err = SYS_GetError();

	for (i = 0; i < 32; i++)
	{
		switch(i)			//bit number - we could also use the mask
		{
		case 0:
			if (err & mask)
				print_log(out, SYS_ERR_B0_STR);
			break;
		case 1:
			if (err & mask)
				print_log(out, SYS_ERR_B1_STR);
			break;
		case 2:
			if (err & mask)
				print_log(out, SYS_ERR_B2_STR);
			break;
		case 3:
			if (err & mask)
				print_log(out, SYS_ERR_B3_STR);
			break;
		case 4:
			if (err & mask)
				print_log(out, SYS_ERR_B4_STR);
			break;
		case 5:
			if (err & mask)
				print_log(out, SYS_ERR_B5_STR);
			break;
		case 6:
			if (err & mask)
				print_log(out, SYS_ERR_B6_STR);
			break;
		case 7:
			if (err & mask)
				print_log(out, SYS_ERR_B7_STR);
			break;
		case 8:
			if (err & mask)
				print_log(out, SYS_ERR_B8_STR);
			break;

		case 16:
			if (err & mask)
				print_log(out, SYS_ERR_B16_STR);
			break;
		case 17:
			if (err & mask)
				print_log(out, SYS_ERR_B17_STR);
			break;

		case 20:
			if (err & mask)
				print_log(out, SYS_ERR_B20_STR);
			break;
		case 21:
			if (err & mask)
				print_log(out, SYS_ERR_B21_STR);
			break;
		case 22:
			if (err & mask)
				print_log(out, SYS_ERR_B22_STR);
			break;
		case 23:
			if (err & mask)
				print_log(out, SYS_ERR_B23_STR);
			break;

		case 31:
			if (err & mask)
				print_log(out, SYS_ERR_B31_STR);
			break;
		}

		mask <<= 1;
	}
}
