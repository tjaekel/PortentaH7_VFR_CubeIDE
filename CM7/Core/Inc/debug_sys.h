/**
 * debug_sys.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef INC_DEBUG_SYS_H_
#define INC_DEBUG_SYS_H_

#include <string.h>
#include "stm32h747xx.h"

#include "VCP_UART.h"
#include "syserr.h"

//DEBUG flags
#define DBG_VERBOSE			0x00000001				//verbose mode, e.g. network trace logs
#define DBG_LOG_BOTH		0x00000002				//all commands on network, log also on UART
#define DBG_SHOW_SPI		0x00000004
#define	DBG_NETWORK			0x00000008				//network Rx debug
#define	DBG_I2C				0x00000010
#define DBG_NET_BYTE		0x00000020				//network SPI transaction as byte dump
#define DBG_NET_12BIT		0x00000040				//network SPI transaction as 12bit dump
#define DBG_CSB_LOW			0x80000000				//keep CSB low

//CONFIG flags
//key:
#define CFG_KEY_VALID		0xC5ACCE55u
//NetworkCfg:
#define						CFG_NET_STATIC		0x00000001
#define						CFG_NET_DYNAMIC		0x00000000
//SPImode:
#define						CFG_SPI_MODE		0x0000000F		//0..3
#define						CFG_SPI_1STBIT		0x000000F0		//0 LSB or !=0, e.g. 7 for MSB
#define						CFG_SPI_WORD		0x00000F00		//8, 16, 32 for bits per word: use 1 = byte, 2 = two bytes, 4 = four bytes
#define						CFG_SPI_CFG_ENDIAN	0x0000F000		//0 : little, 1 : big endian SPI words
//SPIcfg:
#define						CFG_SPI_CFG_MSSI	0x0000000F		//4b value
#define						CFG_SPI_CFG_MIDI	0x000000F0		//4b value
#define						CFG_SPI_CFG_EARLY	0x00000F00		//0 or 1
#define						CFG_SPI_CFG_HWNSS	0x0000F000		//0 or 1
#define						CFG_SPI_CFG_WSIZE	0x00FF0000		//bit size 4..32 - 1! (internally, use a bitNum -1!)

typedef struct {
	unsigned long			key;					// 0: key, if valid config
	unsigned long			NetworkCfg;				// 1: dynamic or static IP address
	unsigned long			SPIbitrate;				// 2: as set, in KHz
	unsigned long			SPImode;				// 3: mode, LSB/MSB, word size
	unsigned long			SPIcfg;					// 4: bit 23..16: word size, bit 12: HW/SW NSS, bit 8: early NSS on network, timing config: IDLE: 4b + NSS: 4b
	unsigned long			I2Cbitrate;				// 5: enum 0..2
	unsigned long			I2CslaveAddr;			// 6: slave addr 7b, shifted down >>1
	unsigned long			I2CslaveAddr2;			// 7:
	unsigned long			GPIOdir;				// 8: GPIO direction
	unsigned long			GPIOod;					// 9: GPIO open drain mask
	unsigned long			GPIOval;				//10: GPIO output val
	unsigned long			GPIOmask;				//11: which outputs to set
	unsigned long			SPInoop;				//12: SPI noop code (for loopback)
	unsigned long			ChipNo;					//13: SPI protocol, chip number
	unsigned long			SysCfg;					//14: bit 0: = 1: heart beat LED off
	unsigned long			res14;					//15
	unsigned long			res15;					//16
	unsigned long			res16;					//17
	unsigned long			res17;					//18
	unsigned long			res18;					//19: max. (20 registers in RTC)
} tCFGparams;

#define RTC_START_BKP_REG	RTC->BKP11R
#define RTC_NUM_BKP			20

//exported global variables
extern unsigned long GDebug;
extern tCFGparams GSysCfg;
extern unsigned long gIPAddr;

//exported functions
void SYSCFG_Init(void);
void SYSCFG_Default(void);
void SYSCFG_setCfg(int idx, unsigned long val);
unsigned long SYSCFG_getCfg(int idx);
void SYSCFG_print(EResultOut out);

#endif /* INC_DEBUG_SYS_H_ */
