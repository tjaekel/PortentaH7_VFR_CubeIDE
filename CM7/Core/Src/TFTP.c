/*
 * TFTP.c
 *
 *  Created on: Feb 8, 2021
 *      Author: Torsten Jaekel
 */

#include "TFTP.h"

#include "SDCard.h"

#include "VCP_UART.h"		//for print_log
#include "MEM_Pool.h"
#include "syserr.h"

static FIL tftpFileHandle;
static char *PLbuf;
/**
 * ATT: we had to transfer from/to SDCard via a buffer, dynamically allocated
 */

void* TFTP_open(const char* fname, const char* mode, u8_t write)
{
	(void)mode;							//do we need to know of 'netascii' or 'octet'?
	FRESULT res;

	if (SDCard_GetStatus())
	{
		/* open a file, use the write flag for creating new file or open for read */
		/*
		 * return a valid file pointer handle (not NULL), not used in LwIP TFTP just for error, just forwarded
		 * to the Read, Write and Close file system functions
		 */
		PLbuf = (char *)MEM_PoolAlloc(MEM_POOL_SEG_SIZE);
		if ( ! PLbuf)
		{
			return NULL;
		}

		if ( ! write)
		{
			if(f_open(&tftpFileHandle, fname, FA_READ) == FR_OK)
			{
				return (void *)&tftpFileHandle;
			}
			else
			{
				SYS_SetError(SYS_ERR_NO_FILE);
				MEM_PoolFree(PLbuf);
				return NULL;
			}
		}
		else
		{
			//open for write (overwrite)
			res = f_open(&tftpFileHandle, fname, FA_CREATE_ALWAYS | FA_WRITE);
			if (res == FR_OK)
			{
				return (void *)&tftpFileHandle;
			}
			else
			{
				SYS_SetError(SYS_ERR_NO_FILE);
				MEM_PoolFree(PLbuf);
				return NULL;
			}
		}
	}
	else
	{
		SYS_SetError(SYS_ERR_SDNOTOPEN);
		return NULL;						//SDCard is not initialized or any error
	}
}

void  TFTP_close(void* handle)
{
	/* close the file handle */
	if (handle != NULL)
		f_close(handle);
	MEM_PoolFree(PLbuf);
}

int   TFTP_read(void* handle, void* buf, int bytes)
{
	unsigned int numRd;
	FRESULT res;

	/* function returns the number of bytes read, e.g. a full chunk, or shorter last chunk,
	 * or <0 for EOF
	 * ATT: we had to use a buffer and memcpy - a direct transfer between ETH and SDCard fails!
	 */
	res = f_read(handle, PLbuf, bytes, &numRd);
	if (res != FR_OK)
	{
		SYS_SetError(SYS_ERR_SDERROR);
		numRd = 0;
	}
	else
		memcpy(buf, PLbuf, numRd);

	if (numRd > 0)
		return numRd;			//how many bytes were available, could be read
	else
	{
		SYS_SetError(SYS_ERR_SDERROR);
		return -1;				//EOF
	}
}

int   TFTP_write(void* handle, struct pbuf* p)
{
	unsigned int numWr = 0;
	FRESULT res;

	////print_log(UART_OUT, "W: %d | %d\r\n", p->len, numWr);
	memcpy(PLbuf, p->payload, p->len);
	/* return how many bytes were written */
	res = f_write(handle, PLbuf, p->len, &numWr);
	if (res != FR_OK)
	{
			SYS_SetError(SYS_ERR_SDERROR);
			return -1;
	}

	if (numWr > 0)
		return numWr;
	else
	{
		SYS_SetError(SYS_ERR_SDERROR);
		return -1;
	}
}

/* initialize the TFTP handler functions for the file system */
const struct tftp_context TFTPctx = {
		TFTP_open,
		TFTP_close,
		TFTP_read,
		TFTP_write
};

