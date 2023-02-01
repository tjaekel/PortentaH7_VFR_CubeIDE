/*
 * CHIP_cmd.h
 *
 *  Created on: Oct 6, 2022
 *      Author: tj
 */

#ifndef INC_CHIP_CMD_H_
#define INC_CHIP_CMD_H_

#include <stdint.h>

/* define the command codes */
#define	SPI_RREG_CMD					0x0001
#define	SPI_WREG_CMD					0x0002
#define SPI_RBLK_CMD					0x0007
#define SPI_WBLK_CMD					0x0003
#define SPI_NOOP_CMD					0x0000

/* REG addresses are multiples of 2 (always even)
 * we use address w/o bit 0 (assumed as 0)
 */
#define SPI_REG_ADDR_SHIFT(a)			(a << 1)
#define SPI_REG_ADDR_MASK(a)			(a & 0x7FFE)
#define SPI_BLK_ADDR_SHIFT(a)			(a << 8)
#define SPI_BLK_ADDR_MASK(a)			(a & 0x7)

#define SPI_CHIPID_ADDR					0x0002
#define SPI_CHIPID_VALUE				0x15A0

/* for handling 384bit slices */
#define BITS_PER_SLICE	384
#define NUM_WORD_SLICE	((BITS_PER_SLICE / 8) / 2)
#define NUM_BYTE_SLICE	(BITS_PER_SLICE / 8)
/* based on max. 64KB SPI chunk: */
#define MAX_NUM_SLICE	((64*1024 - 2) / NUM_BYTE_SLICE)

//for all chips:
void CHIP_SwapEndian(uint8_t *buf, int len);				//for all chips: swap the bytes for endian

/* depending on chips: but our generic interface:
 * provide the buffers on call
 * make sure the buffers contain the right SPI word format, e.g. uint16_t words
 */
int CHIP_cid(uint32_t *cid);								//read ChipID
int CHIP_rreg(uint32_t addr, uint32_t *values, int len);
int CHIP_wreg(uint32_t addr, uint32_t *values, int len);
int CHIP_rblk(uint32_t addr, uint32_t *values, int len, int option);
int CHIP_wblk(uint32_t addr, uint32_t *values, int len, int option);
int CHIP_noop(uint32_t code, int num);
int CHIP_rslice(uint32_t addr, int num, int option);
int CHIP_wslice(uint32_t addr, uint32_t *values, int num, int option);
int CHIP_fslice(uint32_t addr, uint32_t val, int num, int option);

#endif /* INC_CHIP_CMD_H_ */
