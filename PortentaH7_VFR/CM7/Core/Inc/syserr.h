/**
 * syserr.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef __SYSERR_H__
#define __SYSERR_H__

#include "VCP_UART.h"

/**
 * system error values and global variable:
 * we or the codes together in order to see several different errors occured
 */

#define	SYS_ERR_OUT_OF_MEM		0x00000001U
#define SYS_ERR_B0_STR			"Out of memory\r\n"
#define	SYS_ERR_SYS_INIT		0x00000002U
#define SYS_ERR_B1_STR			"System init failed\r\n"
#define SYS_ERR_INVALID_CMD		0x00000004U
#define SYS_ERR_B2_STR			"Invalid Command\r\n"
#define	SYS_ERR_SYSCFG			0x00000008U
#define SYS_ERR_B3_STR			"SYSCFG failed\r\n"
#define SYS_ERR_NETWORK			0x00000010U
#define SYS_ERR_B4_STR			"Network error\r\n"
#define	SYS_ERR_DHCPTO			0x00000020U
#define SYS_ERR_B5_STR			"DHCP Timeout\r\n"
#define SYS_ERR_ETHDOWN			0x00000040U
#define	SYS_ERR_B6_STR			"ETH link down\r\n"
#define SYS_ERR_INVPARAM		0x00000080U
#define	SYS_ERR_B7_STR			"Invalid cmd parameter\r\n"
#define SYS_ERR_OVERFLOW		0x00000100U
#define SYS_ERR_B8_STR			"Buffer overflow\r\n"

/* MEM_PoolFree with NULL pointer */
#define	SYS_ERR_NULL_PTR		0x00010000U		//try to release a NULL pointer
#define SYS_ERR_B16_STR			"NULL pointer release\r\n"
#define SYS_ERR_INV_PTR			0x00020000U		//release a mem pointer not found
#define SYS_ERR_B17_STR			"Invalid pointer release\r\n"


#define	SYS_ERR_STATICIP		0x00100000U
#define SYS_ERR_B20_STR			"Static IP used\r\n"

#define SYS_ERR_NO_FILE			0x00200000U
#define SYS_ERR_B21_STR			"SDCard no file\r\n"
#define	SYS_ERR_SDNOTOPEN		0x00400000U
#define SYS_ERR_B22_STR			"SDCard not open\r\n"
#define SYS_ERR_SDERROR			0x00800000U
#define SYS_ERR_B23_STR			"SDCard error\r\n"

/* FATAL Error */
#define	SYS_ERR_FATAL			0x80000000U		//not usable - dead forever
#define SYS_ERR_B31_STR			"FATAL error\r\n"

extern unsigned long GSysErr;

extern void SYS_SetError(unsigned long errCode);
extern unsigned long SYS_GetError(void);
extern void SYS_ClrError(void);
void SYS_DisplayError(EResultOut out);

#endif
