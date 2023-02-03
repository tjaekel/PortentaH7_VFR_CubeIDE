/**
 * debug_sys.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#include "debug_sys.h"

unsigned long GDebug  = 0l;			//default could be 0x1l - for VERBOSE

/* we have max. 32 32bit backup registers in RTC */
tCFGparams GSysCfg = {
		.key				= CFG_KEY_VALID,			//0
		.NetworkCfg			= CFG_NET_DYNAMIC,			//1 : CFG_NET_STATIC: we set default to static: 10.59.144.26
		.SPIbitrate			= 10000,					//2
		.SPImode			= 0x0200,					//3 : 2bytes, LSB first, SPI mode 0
		.SPIcfg				= 0x00071100,				//4 : wordSize, HWSS, Early, IDLE, NSS
		.I2Cbitrate			= 1,						//5
		.I2CslaveAddr		= 0x08,						//6
		.GPIOdir			= 0x00000000ul,				//7
		.GPIOod				= 0x00000000ul,				//8
		.GPIOval			= 0x00000000ul,				//9
		.GPIOmask			= 0xFFFFFFFFul,				//10
		.SPInoop			= 0x0ul,					//11
		.ChipNo				= 1							//12
};

void SYSCFG_Init(void)
{
	//make we sure have just 32 registers in RTC

	/* check if we have key in RTC BKP0R - if so, copy back from there */
	if (RTC_START_BKP_REG == CFG_KEY_VALID)
	{
		/* copy from RTC to syscfg */
		memcpy(&GSysCfg, (void *)&RTC_START_BKP_REG, RTC_NUM_BKP * 4);
	}
	else
	{
		/* copy default to RTC BKPxx registers */
		memcpy((void *)&RTC_START_BKP_REG, &GSysCfg, RTC_NUM_BKP * 4);
		/* check if key is there */
		if (RTC_START_BKP_REG != CFG_KEY_VALID)
			SYS_SetError(SYS_ERR_SYSCFG);
	}
}

void SYSCFG_setCfg(int idx, unsigned long val)
{
	unsigned long *cfg;
	if (idx > RTC_NUM_BKP)
	{
		SYS_SetError(SYS_ERR_INVPARAM);
		return;				//outside struct
	}
	if (idx == 0)
	{
		SYS_SetError(SYS_ERR_INVPARAM);
		return;				//do not touch the key
	}

	cfg = (unsigned long *)&GSysCfg;
	*(cfg + idx) = val;
	cfg = (unsigned long *)&RTC_START_BKP_REG;
	*(cfg + idx) = val;
}

unsigned long SYSCFG_getCfg(int idx)
{
	unsigned long *cfg;
	if (idx > RTC_NUM_BKP)
	{
		SYS_SetError(SYS_ERR_INVPARAM);
		return 0;				//outside struct
	}
	if (idx == 0)
	{
		SYS_SetError(SYS_ERR_INVPARAM);
		return 0;				//do not touch the key
	}

	cfg = (unsigned long *)&RTC_START_BKP_REG;
	return (*(cfg + idx));
}

void SYSCFG_print(EResultOut out)
{
	//check if the key is valid
	if (RTC_START_BKP_REG == CFG_KEY_VALID)
	{
		uint32_t *rtcBkpReg = (uint32_t *)&RTC_START_BKP_REG;
		rtcBkpReg++;			//skip the key
		/* we print the RTC backup registers */
		print_log(out, " 1 : Network config : 0x%01lx\r\n", *rtcBkpReg++);
		print_log(out, " 2 : SPIbitrate     : %ld\r\n", 	*rtcBkpReg++);
		print_log(out, " 3 : SPIMode        : 0x%04lx\r\n", *rtcBkpReg++);
		print_log(out, " 4 : SPIcfg         : 0x%04lx\r\n", *rtcBkpReg++);
		print_log(out, " 5 : I2Cbitrate     : %ld\r\n", 	*rtcBkpReg++);
		print_log(out, " 6 : I2CslaveAddr   : 0x%02lx\r\n", *rtcBkpReg++);
		print_log(out, " 7 : GPIOdir        : 0x%02lx\r\n", *rtcBkpReg++);
		print_log(out, " 8 : GPIOod         : 0x%02lx\r\n", *rtcBkpReg++);
		print_log(out, " 9 : GPIOval        : 0x%02lx\r\n", *rtcBkpReg++);
		print_log(out, "10 : GPIOmask       : 0x%02lx\r\n", *rtcBkpReg++);
		print_log(out, "11 : SPI NOOP       : 0x%08lx\r\n", *rtcBkpReg++);
		print_log(out, "12 : Chip Number    : %ld\r\n", 	*rtcBkpReg++);
	}
}
