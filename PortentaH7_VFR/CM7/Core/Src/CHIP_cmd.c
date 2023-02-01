/*
 * CHIP_cmd.c
 *
 *  Created on: Oct 6, 2022
 *      Author: tj
 */

#include "CHIP_cmd.h"
#include "debug_sys.h"

#include "cmd_dec.h"
#include "SPI.h"

void CHIP_SwapEndian(uint8_t *buf, int len)
{
	if ((GSysCfg.SPImode & CFG_SPI_CFG_ENDIAN) >> 12)
	{
		//swap the bytes
		if (((GSysCfg.SPImode & CFG_SPI_WORD) >> 8) == 1)
			return;		//nothing on bytes
		if (((GSysCfg.SPImode & CFG_SPI_WORD) >> 8) == 2)
		{
			//2 byte swap
			uint8_t tmp;
			while (len > 0)
			{
				tmp = *buf;
				*buf = *(buf + 1);
				*(buf + 1) = tmp;

				len -= 2;
				buf += 2;
			}
		}
		if (((GSysCfg.SPImode & CFG_SPI_WORD) >> 8) == 4)
		{
			//4 byte swap
			uint8_t tmp;
			while (len > 0)
			{
				tmp = *buf;
				*buf = *(buf + 3);
				*(buf + 3) = tmp;

				tmp = *(buf + 1);
				*(buf + 1) = *(buf + 2);
				*(buf + 2) = tmp;

				len -= 4;
				buf += 4;
			}
		}
	}
	//leave it at it is
}

/* ---------------------------------------------------------------------- */

int CHIP_cid(uint32_t *cid)
{
	if (GSysCfg.ChipNo == 0)
	{
		uint32_t *spiTx = (uint32_t *)SPIbuf.spiTx;
		spiTx[0] = 0x00040580ul;
		spiTx[1] = 0x400B0FD0ul;
		spiTx[2] = 0xAAA00000ul;
		spiTx[3] = 0ul;
		spiTx[4] = GSysCfg.SPInoop;		//must be 0 on real chip
		spiTx[5] = 0ul;

		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, 6*4);

		*cid  = (unsigned long)SPIbuf.spiRx[16] <<  0;
		*cid |= (unsigned long)SPIbuf.spiRx[17] <<  8;
		*cid |= (unsigned long)SPIbuf.spiRx[18] << 16;
		*cid |= (unsigned long)SPIbuf.spiRx[19] << 24;

		if (GDebug & DBG_VERBOSE)
			hex_dump(SPIbuf.spiRx, 6*4, 4, UART_OUT);		//len in number of bytes
		return 1;
	}

	if (GSysCfg.ChipNo == 1)
	{
		uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
		*spiTx    = SPI_REG_ADDR_SHIFT(SPI_REG_ADDR_MASK(SPI_CHIPID_ADDR));
		*spiTx++ |= SPI_RREG_CMD;
		*spiTx    = GSysCfg.SPInoop;

		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, 2*2);

		*cid  = SPIbuf.spiRx[2];
		*cid |= SPIbuf.spiRx[3] << 8;

		if (GDebug & DBG_VERBOSE)
		{
			print_log(UART_OUT, "*D Rx:\r\n");
			hex_dump(SPIbuf.spiRx, 2*2, 2, UART_OUT);		//len in number of bytes
		}
		return 1;
	}

	return 0;
}

int CHIP_rreg(uint32_t addr, uint32_t *values, int len)
{
	/* ATTENTIO: this chip does not support consecutive read
	 * therefore, read single registers!
	 */
	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	int xLen;
#if 0
	*spiTx    = SPI_REG_ADDR_SHIFT(SPI_REG_ADDR_MASK(addr));
	*spiTx++ |= SPI_RREG_CMD;
#endif
	if ( ! len)
		len = 1;
	if (len > CMD_DEC_NUM_VAL)				//avoid buffer overflow
		len = CMD_DEC_NUM_VAL;
	xLen = len;
#if 0  //consecutive not supported!
	while (len--)
	{
		*spiTx++ = GSysCfg.SPInoop;
	}
	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen*2 + 2);
#else
	{
		static uint16_t rxBuf[2];		//ATTENTION: cannot be on DTCM!
		uint16_t *spiRx = (uint16_t *)SPIbuf.spiRx;
		int i = 0;

		while (len--)
		{
			*spiTx  = SPI_REG_ADDR_SHIFT(SPI_REG_ADDR_MASK(addr));
			*spiTx |= SPI_RREG_CMD;
			*(spiTx + 1) = GSysCfg.SPInoop;
			SPI_TransmitReceive(SPIbuf.spiTx, (uint8_t *)rxBuf, 2 + 2);

			if (GDebug & DBG_VERBOSE)
			{
				print_log(UART_OUT, "*D Tx: %04x | Rx: %04x\r\n", *spiTx, rxBuf[1]);
			}

			if (i == 0)
				*spiRx = rxBuf[0];

			*(spiRx + 1 + i) = rxBuf[1];
			i++;
			addr += 2;
		}
	}
#endif

#if 0
	if (GDebug & DBG_VERBOSE)
	{
		print_log(UART_OUT, "*D:\r\n");
		hex_dump(SPIbuf.spiRx, xLen*2 + 2, 2, UART_OUT);		//len in number of bytes
	}
#endif

	//not optimal! avoid to copy, but how w/o dependencies on caller
	memcpy(values, &SPIbuf.spiRx[2], xLen * 2);

	return xLen;					//OK, number of values
}

int CHIP_wreg(uint32_t addr, uint32_t *values, int len)
{
	/* values is array with 32bit words! */
	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	int xLen;
#if 0
	//just if we can do consecutive
	*spiTx    = SPI_REG_ADDR_SHIFT(SPI_REG_ADDR_MASK(addr));
	*spiTx++ |= SPI_WREG_CMD;
	if ( ! len)
		len = 1;
	xLen = len;

	while (len--)
	{
		*spiTx++ = (uint16_t)*values++;
	}
	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen*2 + 2);

	if (GDebug & DBG_VERBOSE)
	{
		print_log(UART_OUT, "*D:\r\n");
		hex_dump(SPIbuf.spiRx, xLen*2 + 2, 2, UART_OUT);		//len in number of bytes
	}
#else
	if ( ! len)
		len = 1;
	xLen = len;
	while (len--)
	{
		*spiTx  = SPI_REG_ADDR_SHIFT(SPI_REG_ADDR_MASK(addr));
		*spiTx |= SPI_WREG_CMD;
		*(spiTx + 1) = (uint16_t)*values++;

		if (GDebug & DBG_VERBOSE)
		{
			print_log(UART_OUT, "*D Tx: %04x %04x\r\n", *spiTx, *(spiTx + 1));
		}

		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, 2 + 2);
		addr += 2;
	}
#endif

	return xLen;					//OK, number of values
}

int CHIP_rblk(uint32_t addr, uint32_t *values, int len, int option)
{
	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	int xLen;
	*spiTx    = SPI_BLK_ADDR_SHIFT(SPI_BLK_ADDR_MASK(addr));
	*spiTx++ |= SPI_RBLK_CMD;
	if ( ! len)
		len = 1;
	if (len > CMD_DEC_NUM_VAL)				//avoid buffer overflow
		len = CMD_DEC_NUM_VAL;
	xLen = len;
	while (len--)
	{
		//not optimized for option
		*spiTx++ = GSysCfg.SPInoop;
	}
	if (option == 0)
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen*2 + 2);
	if (option == 1)
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen + 2);
	if (option == 3)
		//12bit: it is
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, (xLen*3)/2 + 2);

	if (GDebug & DBG_VERBOSE)
	{
		print_log(UART_OUT, "*D: Rx\r\n");
		//always as byte dump
		if (option == 0)
			hex_dump(SPIbuf.spiRx, xLen*2 + 2, 1, UART_OUT);		//len in number of bytes
		if (option == 1)
			hex_dump(SPIbuf.spiRx, xLen + 2, 1, UART_OUT);			//len in number of bytes
		if (option == 2)
			hex_dump(SPIbuf.spiRx, (xLen*3)/2 + 2, 1, UART_OUT);	//len in number of bytes
	}

	//not optimal! avoid to copy, but how w/o dependencies on caller
	if (option == 0)
		memcpy(values, &SPIbuf.spiRx[2], xLen * 2);
	if (option == 1)
		memcpy(values, &SPIbuf.spiRx[2], xLen);
	if (option == 2)
		memcpy(values, &SPIbuf.spiRx[2], (xLen*3)/2);

	return xLen;				//OK, number of values
}

int CHIP_wblk(uint32_t addr, uint32_t *values, int len, int option)
{
	/* values is array with 32bit words! */
	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	uint8_t *spiTxB;
	int xLen;
	*spiTx    = SPI_BLK_ADDR_SHIFT(SPI_BLK_ADDR_MASK(addr));
	*spiTx++ |= SPI_WBLK_CMD;
	spiTxB = (uint8_t *)spiTx;

	if ( ! len)
		len = 1;
	xLen = len;

	while (len > 0)
	{
		if (option == 0)
			*spiTx++ = (uint16_t)*values++;
		if (option == 1)
			*spiTxB++ = (uint8_t)*values++;
		if (option == 2)
		{
			//as 12bit values
			*spiTxB++   = (uint8_t)*values;
			*spiTxB     = (*values >> 8) & 0xF;
			*spiTxB++  |= (*(values +1) & 0xF) << 4;
			*spiTxB++   = *(values + 1) >> 4;
			values += 2;
		}
		len--;
	}

	if (option == 0)
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen*2 + 2);
	if (option == 1)
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen + 2);
	if (option == 2)
		SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, (xLen*3)/2 + 2);

	if (GDebug & DBG_VERBOSE)
	{
		print_log(UART_OUT, "*D: Tx\r\n");
		//always as byte dump
		if (option == 0)
			hex_dump(SPIbuf.spiTx, xLen*2 + 2, 1, UART_OUT);		//len in number of bytes
		if (option == 1)
			hex_dump(SPIbuf.spiTx, xLen + 2, 1, UART_OUT);			//len in number of bytes
		if (option == 2)
			hex_dump(SPIbuf.spiTx, (xLen*3)/2 + 2, 1, UART_OUT);	//len in number of bytes
	}

	return len;					//OK, number of values
}

int CHIP_noop(uint32_t code, int num)
{
	(void)code;

	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	int xNum;
	*spiTx++ = SPI_NOOP_CMD;

	xNum = num;

	while (num--)
	{
		*spiTx++ = GSysCfg.SPInoop;
	}
	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xNum*2 + 2);

	if (GDebug & DBG_VERBOSE)
	{
		print_log(UART_OUT, "*D:\r\n");
		hex_dump(SPIbuf.spiRx, xNum*2 + 2, 2, UART_OUT);		//len in number of bytes
	}

	return xNum;					//OK, number of NOOPs done
}

int CHIP_rslice(uint32_t addr, int num, int option)
{
	/* read num of slices, each slice as 384bits = 24words = 48byte */
	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	int xLen;
	*spiTx    = SPI_BLK_ADDR_SHIFT(SPI_BLK_ADDR_MASK(addr));
	*spiTx++ |= SPI_RBLK_CMD;
	if ( ! num)
		num = 1;
	if ((num * NUM_BYTE_SLICE) > (SPI_MAX_LEN_CMDLINE - 2))				//avoid buffer overflow
		num = (SPI_MAX_LEN_CMDLINE - 2) / NUM_BYTE_SLICE;				//43 slices is maximum
	xLen = num;
	while (num--)
	{
		int i;
		for (i = 0; i < NUM_WORD_SLICE; i++)
			*spiTx++ = GSysCfg.SPInoop;
	}
	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen*NUM_WORD_SLICE*2 + 2);

	if (option == 0)
		hex_dump(SPIbuf.spiRx + 2, xLen*NUM_WORD_SLICE*2, 2, UART_OUT);		//len in number of bytes
	if (option == 1)
		hex_dump(SPIbuf.spiRx + 2, xLen*NUM_WORD_SLICE*2, 1, UART_OUT);		//len in number of bytes
	if (option == 2)
		hex_dump(SPIbuf.spiRx + 2, xLen*NUM_WORD_SLICE*2, 3, UART_OUT);		//len in number of bytes

	return xLen;
}

int CHIP_wslice(uint32_t addr, uint32_t *values, int num, int option)
{
	/* write num of slices where each slice gets the same value */
	/* values is array with 32bit words! */
	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	uint8_t *spiTxB;

	int xLen;
	*spiTx    = SPI_BLK_ADDR_SHIFT(SPI_BLK_ADDR_MASK(addr));
	*spiTx++ |= SPI_WBLK_CMD;
	spiTxB = (uint8_t *)spiTx;

	if ( ! num)
		num = 1;
	if ((num * NUM_BYTE_SLICE) > (SPI_MAX_LEN_CMDLINE - 2))				//avoid buffer overflow
		num = (SPI_MAX_LEN_CMDLINE - 2) / NUM_BYTE_SLICE;				//43 slices is maximum
	xLen = num;
	while (num--)
	{
		int i;
		if (option == 0)
		{
			for (i = 0; i < NUM_WORD_SLICE; i++)
				*spiTx++ = (uint16_t)*values;
		}
		if (option == 1)
		{
			for (i = 0; i < (NUM_WORD_SLICE * 2); i++)
				*spiTxB++ = (uint8_t)*values;
		}
		if (option == 2)
		{
			//write 12bit values
			for (i = 0; i < (NUM_WORD_SLICE * 2); i = i+3)
			{
				*spiTxB++   = (uint8_t)*values;
				*spiTxB     = (*values >> 8) & 0xF;
				*spiTxB++  |= (*values & 0xF) << 4;
				*spiTxB++   = *values >> 4;
			}
		}
		values++;
	}
	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen*NUM_WORD_SLICE*2 + 2);

	if (GDebug & DBG_VERBOSE)
	{
		print_log(UART_OUT, "*D: Tx\r\n");
			//always as SPI Tx bytes
			hex_dump(SPIbuf.spiTx, xLen*NUM_WORD_SLICE*2 + 2, 1, UART_OUT);		//len in number of bytes
	}

	return xLen;
}

int CHIP_fslice(uint32_t addr, uint32_t val, int num, int option)
{
	/* fill slice with incrementing pattern, val is starting fill pattern */
	uint16_t *spiTx = (uint16_t *)SPIbuf.spiTx;
	uint8_t *spiTxB;
	int xLen;
	*spiTx    = SPI_BLK_ADDR_SHIFT(SPI_BLK_ADDR_MASK(addr));
	*spiTx++ |= SPI_WBLK_CMD;
	spiTxB = (uint8_t *)spiTx;

	if ( ! num)
		num = 1;
	if ((num * NUM_BYTE_SLICE) > (SPI_MAX_LEN_CMDLINE - 2))				//avoid buffer overflow
		num = (SPI_MAX_LEN_CMDLINE - 2) / NUM_BYTE_SLICE;				//43 slices is maximum
	xLen = num;
	while (num--)
	{
		int i;
		if (option == 0)
		{
			for (i = 0; i < NUM_WORD_SLICE; i++)
				*spiTx++ = (uint16_t)val;
		}
		if (option == 1)
		{
			for (i = 0; i < (NUM_WORD_SLICE * 2); i++)
				*spiTxB++ = (uint8_t)val;
		}
		if (option == 2)
		{
			//write 12bit values
			for (i = 0; i < (NUM_WORD_SLICE * 2); i = i+3)
			{
				*spiTxB++   = (uint8_t)val;
				*spiTxB     = (val >> 8) & 0xF;
				*spiTxB++  |= (val & 0xF) << 4;
				*spiTxB++   = val >> 4;
			}
		}
		val++;
	}
	SPI_TransmitReceive(SPIbuf.spiTx, SPIbuf.spiRx, xLen*NUM_WORD_SLICE*2 + 2);

	if (GDebug & DBG_VERBOSE)
	{
		print_log(UART_OUT, "*D: Tx\r\n");
			//always as SPI Tx bytes
			hex_dump(SPIbuf.spiTx, xLen*NUM_WORD_SLICE*2 + 2, 1, UART_OUT);		//len in number of bytes
	}

	return xLen;
}
