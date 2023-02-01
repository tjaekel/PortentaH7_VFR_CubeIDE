/**
 * spi.h
 *
 *  Created on: May 3, 2018
 *  Author: Torsten Jaekel
 */

#ifndef SPI_H_
#define SPI_H_

#include "stm32h7xx_hal.h"
#include <cmsis_os.h>
#include <semphr.h>
#include "main.h"
#include "VCP_UART.h"
#include "debug_sys.h"
#include "cmd_dec.h"

#define SW_NSS2							//use SW NSS - much shorter and faster back-to-back

/* Definition for SPIx's DMA */
#define DMA_SPI_CLK_ENABLE()             __HAL_RCC_DMA2_CLK_ENABLE()

#define SPI2_TX_DMA_STREAM               DMA2_Stream3
#define SPI2_RX_DMA_STREAM               DMA2_Stream2

#define SPI2_TX_DMA_REQUEST              DMA_REQUEST_SPI2_TX
#define SPI2_RX_DMA_REQUEST              DMA_REQUEST_SPI2_RX

/* Definition for SPIx's NVIC */
#define SPI2_DMA_TX_IRQn                 DMA2_Stream3_IRQn
#define SPI2_DMA_RX_IRQn                 DMA2_Stream2_IRQn

#define SPI2_DMA_TX_IRQHandler           DMA2_Stream3_IRQHandler
#define SPI2_DMA_RX_IRQHandler           DMA2_Stream2_IRQHandler

//macros for INTs and INThandlers
#define SPIx2_IRQn                       SPI2_IRQn
#define SPIx2_IRQHandler                 SPI2_IRQHandler

extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_spi2tx;
extern DMA_HandleTypeDef hdma_spi2rx;

extern osSemaphoreId xSemaphoreSPI2;
extern osSemaphoreId xSemaphoreSPI2Rx;

#define CFG_SPI_1STBIT_LSB		0
#define CFG_SPI_1STBIT_MSB		7

#define SPI_TOTAL_LEN			(64 * 1024 - 1)					//the maximum length of a raw SPI packet - HW limitation: 16bit DMA length !
#define SPI_CHIP_TOTAL_LEN		(64 * 1024 + 16)

/**
 * NOTE: make sure SPI_MAX_LEN is >= 4 * CMD_DEC_NUM_VAL + CMD_header !
 * this is SPI buffer used on command line, size is set by CMD_DEC_NUM_VAL
 */
#define	SPI_MAX_LEN_CMDLINE						(4 * CMD_DEC_NUM_VAL + 16)			//chip specific

typedef struct {
	uint8_t spiTx[SPI_MAX_LEN_CMDLINE] __ALIGNED(32);
	uint8_t spiRx[SPI_MAX_LEN_CMDLINE] __ALIGNED(32);
} TSPIBuffer;

extern TSPIBuffer SPIbuf;

void SPI_Init(void);
int SPI_TransmitReceive(uint8_t *txBuf, uint8_t *rxBuf, uint32_t len);
int SPI_SetConfig(int khzFreq);
int SPI_GetClock(void);

#endif /* SPI_H_ */
