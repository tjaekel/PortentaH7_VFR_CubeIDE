/**
 * MEM_Pool.c
 *
 *  Created on: Feb 12, 2017
 *  Author: Torsten Jaekel
 */

//#include <stdio.h>
#define	NULL	(void *)0
#include "MEM_Pool.h"
#include "syserr.h"

#include "VCP_UART.h"

uint8_t sMemPoolRAM[MEM_POOL_SEG_SIZE * MEM_POOL_SEGMENTS];	//MEM_POOL_SEGMENTS chunks
static TMem_Pool gMemPoolMgt[MEM_POOL_SEGMENTS];
static unsigned int lookAheadIdx;

static unsigned int sMEMPoolWatermark;
static unsigned int inUse;

void MEM_PoolInit(void)
{
	//initialize MEM_Pool
	unsigned long i;

#if 1
	/* just to avoid a "not used" warning */
	sMemPoolRAM[0] = 0;
#endif

	for (i= 0; i < MEM_POOL_SEGMENTS; i++)
	{
		gMemPoolMgt[i].startAddr = SRAM_FREE_START + (i * MEM_POOL_SEG_SIZE);
		gMemPoolMgt[i].alloc 	 = ALLOC_FREE;
	}

	sMEMPoolWatermark = 0;
	lookAheadIdx = 0;
	inUse = 0;
}

/*
 * n : in number of bytes - converted into N * MEM_POOL_SEG_SIZE
 */
void * /*__attribute__((section(".itcmram")))*/ MEM_PoolAlloc(unsigned int n)
{
	//allocate N * SEGMENTS as one chunk
	unsigned int i, j, k;
	int f;

	if ( ! n)
		return NULL;

	/* translate into N * MEM_POOL_SEG_SIZE */
	n = (n + MEM_POOL_SEG_SIZE -1) / MEM_POOL_SEG_SIZE;

	f = -1;
	/* start searching from current position
	 * keep the previously used segments still untouched, e.g. for network NO_COPY
	 */
	for (i = lookAheadIdx; i < MEM_POOL_SEGMENTS; i++)
	{
		if (gMemPoolMgt[i].alloc == ALLOC_FREE)
		{
			//check, if enough segments are free
			k = 1;
			for (j = i + 1; (j < MEM_POOL_SEGMENTS) && (j < (i + n)); j++)
			{
				if (gMemPoolMgt[j].alloc != ALLOC_FREE)
				{
					i = j;		//start over to find free region
					break;
				}
				else
					k++;
			}
			if (k == n)
			{
				f = (int)i;			//found free entry for requested segments
				lookAheadIdx = i + 1;
				break;
			}
		}
	}

	//have we reached the end?
	if (lookAheadIdx >= MEM_POOL_SEGMENTS)
		lookAheadIdx = 0;

	//if not yet found in remaining part - start again from begin
	if (f == -1)
	{
		for (i = 0; i < MEM_POOL_SEGMENTS; i++)
		{
			if (gMemPoolMgt[i].alloc == ALLOC_FREE)
			{
				//check, if enough segments are free
				k = 1;
				for (j = i + 1; (j < MEM_POOL_SEGMENTS) && (j < (i + n)); j++)
				{
					if (gMemPoolMgt[j].alloc != ALLOC_FREE)
					{
						i = j;		//start over to find free region
						break;
					}
					else
						k++;
				}
				if (k == n)
				{
					f = (int)i;			//found free entry for requested segments
					lookAheadIdx = i + 1;
					break;
				}
			}
		}
	}

	if (lookAheadIdx >= MEM_POOL_SEGMENTS)
			lookAheadIdx = 0;

	//check if we have found a free range
	if (f != -1)
	{
		//allocate now all the entries requested (based on SEGMENT size)
		i = (unsigned int)f;
		j = i;
		gMemPoolMgt[i].alloc = ALLOC_USED;
		inUse++;;
		for(i++; i < (j + n); i++)
		{
			gMemPoolMgt[i].alloc = ALLOC_SUBSEQ;
			inUse++;
		}

		//watermark: how much is used as maximum
		if (sMEMPoolWatermark < inUse)
			sMEMPoolWatermark = inUse;

		//return the start address of the region
		return((void *)gMemPoolMgt[f].startAddr);
	}
	else
	{
		//set syserr for Out of Memory
		SYS_SetError(SYS_ERR_OUT_OF_MEM);
		print_log(UART_OUT, "\r\n*E: Out of memory\r\n");
		return NULL;		//error, not found a free region
	}
}

void /*__attribute__((section(".itcmram")))*/ MEM_PoolFree(void *ptr)
{
	//free all use segments
	//search first for the address
	unsigned int i, ok;

	ok = 0;

	if ( ! ptr)
	{
		SYS_SetError(SYS_ERR_NULL_PTR);
		return;			//not a valid pointer, e.g. alloc has failed
	}

	for (i = 0; i < MEM_POOL_SEGMENTS; i++)
	{
		if (gMemPoolMgt[i].startAddr == (unsigned long)ptr)
		{
			//found the start address
			//now release all the segments
			gMemPoolMgt[i].alloc = ALLOC_FREE;
			ok = 1;		//we have released the pointer
			inUse--;
			for (i++; i < MEM_POOL_SEGMENTS; i++)
			{
				//the very last segment used cannot be subsequent
				if (gMemPoolMgt[i].alloc == ALLOC_SUBSEQ)
				{
					gMemPoolMgt[i].alloc = ALLOC_FREE;
					inUse--;
				}
				else
				{
					i = MEM_POOL_SEGMENTS;	//break all loops
					break;
				}
			}
		}
	}

	if ( ! ok)
		SYS_SetError(SYS_ERR_INV_PTR);
}

inline unsigned int MEM_PoolWatermark(void)
{
	return sMEMPoolWatermark;
}

inline unsigned int MEM_PoolMax(void)
{
	return MEM_POOL_SEGMENTS;
}

unsigned int MEM_PoolAvailable(void)
{
	unsigned int i,j;

	j = 0;
	for (i = 0; i < MEM_POOL_SEGMENTS; i++)
		if (gMemPoolMgt[i].alloc == ALLOC_FREE)
			j++;
	return j;
}
