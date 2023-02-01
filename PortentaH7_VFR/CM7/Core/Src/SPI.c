/**
 * SPI.c
 *
 *  Created on: May 3, 2018
 *  Author: Torsten Jaekel
 */

#include <cmsis_os.h>
#include <semphr.h>

#include "SPI.h"
#include "syserr.h"

SPI_HandleTypeDef  hspi2;

DMA_HandleTypeDef hdma_spi2tx;
DMA_HandleTypeDef hdma_spi2rx;

osSemaphoreId xSemaphoreSPI2   = NULL;
osSemaphoreId xSemaphoreSPI2Rx = NULL;

int sSPIdivider = 128;						/* valid: 128, ... 8, 4, 2 */

TSPIBuffer SPIbuf = {
	//nothing to initialize for now
};

/* SPI2 init function */
static void MX_SPI2_Init(void)
{
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;

  /* we keep 8bit = byte, WordSize and Endian used on commands */
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  /* use from syscfg the word size - ATT: as numBits - 1, range: 4..32 */
  /* it does not work for 0x0F as 16bit ! */
  //hspi2.Init.DataSize = (GSysCfg.SPIcfg & CFG_SPI_CFG_WSIZE) >> 16;

  int SPImode;
  int SPI1stBit;

  SPImode = GSysCfg.SPImode & CFG_SPI_MODE;
  switch (SPImode)
  {
  case 0: hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  	  	  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  	  	  break;
  case 1: hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  	  	  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  	  	  break;
  case 2: hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  	  	  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  	  	  break;
  default:
	  	  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  	  	  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  	  	  break;
  }
  SPI1stBit = GSysCfg.SPImode & CFG_SPI_1STBIT;
  SPI1stBit >>= 4;
  if (SPI1stBit == CFG_SPI_1STBIT_LSB)
	  hspi2.Init.FirstBit = SPI_FIRSTBIT_LSB;
  else
	  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;

  switch (sSPIdivider)
  {
  case 128 : hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  	   	   break;
  case 64 : hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  	   	   break;
  case 32 : hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
	   	   break;
  case 16 : hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	   	   break;
  case 8 : hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  	  	   break;
  case 4 : hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	   	   break;
  default: hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	       break;
  }

  if ( ! ((GSysCfg.SPIcfg & CFG_SPI_CFG_HWNSS) >> 12))
	  /* we configure below as GPIO output for SW NSS */
	  hspi2.Init.NSS = SPI_NSS_SOFT;
  else
	  /* HW NSS */
	  hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;

  hspi2.Init.MasterSSIdleness = GSysCfg.SPIcfg & CFG_SPI_CFG_MSSI;						/* lowest 4 bit */
  hspi2.Init.MasterInterDataIdleness = GSysCfg.SPIcfg & CFG_SPI_CFG_MIDI;				/* next higher 4 bit */

  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;	//SPI_MASTER_KEEP_IO_STATE_ENABLE; //SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
	  ////Error_Handler();
	  SYS_SetError(SYS_ERR_SYS_INIT);
  }
}

void SPI_Init(void)
{
  MX_SPI2_Init();

  xSemaphoreSPI2   = osSemaphoreNew(1, 1, NULL);
  xSemaphoreSPI2Rx = osSemaphoreNew(1, 0, NULL);

  if (( ! xSemaphoreSPI2) || ( ! xSemaphoreSPI2Rx))
	  SYS_SetError(SYS_ERR_SYS_INIT);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_SPI2_CLK_ENABLE();

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  DMA_SPI_CLK_ENABLE();

  if(hspi->Instance == SPI2)
  {
    /* Peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();

    /**SPI2 GPIO Configuration
    PC2       ------> SPI2_MISO
    PC3       ------> SPI2_MOSI
    PI0       ------> SPI2_NSS
    PI1       ------> SPI2_SCK
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2| GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    ////if ( ! ((GSysCfg.SPIcfg & CFG_SPI_CFG_HWNSS) >> 12))
    {
    	//software NSS - set NSS high
    	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);

    	//change to GPIO mode
    	GPIO_InitStruct.Pin = GPIO_PIN_0;
    	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    	GPIO_InitStruct.Pull = GPIO_NOPULL;
    	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    	HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
    }
    ////else
    if (((GSysCfg.SPIcfg & CFG_SPI_CFG_HWNSS) >> 12))
    {
    	/* ATTENTION: NSS is floating if SPI is not enabled!
    	 * we need a pull-up!
    	 */
    	/* HW NSS */
    	GPIO_InitStruct.Pin = GPIO_PIN_0;
    	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    	GPIO_InitStruct.Pull = GPIO_NOPULL;
    	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    	HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
    }

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    /* Configure the DMA handler for Transmission process */
    hdma_spi2rx.Instance				 = SPI2_RX_DMA_STREAM;
    hdma_spi2rx.Init.Request			 = SPI2_RX_DMA_REQUEST;
    hdma_spi2rx.Init.Direction			 = DMA_PERIPH_TO_MEMORY;
    hdma_spi2rx.Init.PeriphInc			 = DMA_PINC_DISABLE;
    hdma_spi2rx.Init.MemInc				 = DMA_MINC_ENABLE;
    hdma_spi2rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi2rx.Init.MemDataAlignment	 = DMA_MDATAALIGN_BYTE;
    hdma_spi2rx.Init.Mode				 = DMA_NORMAL;
    hdma_spi2rx.Init.Priority			 = DMA_PRIORITY_LOW;
    hdma_spi2rx.Init.FIFOMode			 = DMA_FIFOMODE_ENABLE;
    hdma_spi2rx.Init.FIFOThreshold		 = DMA_FIFO_THRESHOLD_FULL;
    hdma_spi2rx.Init.MemBurst			 = DMA_MBURST_SINGLE;
    hdma_spi2rx.Init.PeriphBurst		 = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&hdma_spi2rx);

    /* Associate the initialized DMA handle to the the SPI handle */
    __HAL_LINKDMA(hspi, hdmarx, hdma_spi2rx);

    hdma_spi2tx.Instance                 = SPI2_TX_DMA_STREAM;
    hdma_spi2tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    hdma_spi2tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_spi2tx.Init.MemBurst            = DMA_MBURST_INC4;
    hdma_spi2tx.Init.PeriphBurst         = DMA_PBURST_INC4;
    hdma_spi2tx.Init.Request             = SPI2_TX_DMA_REQUEST;
    hdma_spi2tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_spi2tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_spi2tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_spi2tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi2tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_spi2tx.Init.Mode                = DMA_NORMAL;
    hdma_spi2tx.Init.Priority            = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_spi2tx);

    /* Associate the initialized DMA handle to the the SPI handle */
    __HAL_LINKDMA(hspi, hdmatx, hdma_spi2tx);

    /* NVIC configuration for DMA transfer complete interrupt (SPI2_TX) */
    HAL_NVIC_SetPriority(SPI2_DMA_TX_IRQn, 14, 1);
    HAL_NVIC_EnableIRQ(SPI2_DMA_TX_IRQn);

    HAL_NVIC_SetPriority(SPI2_DMA_RX_IRQn, 14, 1);
    HAL_NVIC_EnableIRQ(SPI2_DMA_RX_IRQn);

    /* NVIC configuration for SPI transfer complete interrupt (SPI2) */
    HAL_NVIC_SetPriority(SPIx2_IRQn, 14, 0);
    HAL_NVIC_EnableIRQ(SPIx2_IRQn);
  }
}

#if 0
/* we never use and need DeInit - we do via "fwreset" and restart system, save code space */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
  if(hspi->Instance == SPI2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI2_CLK_DISABLE();

    /**SPI2 GPIO Configuration
    PC2_C     ------> SPI2_MISO
    PC3_C     ------> SPI2_MOSI
    PB12      ------> SPI2_NSS
    PD3       ------> SPI2_SCK
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2|GPIO_PIN_3);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);
  }
}
#endif

static void __attribute__((section(".itcmram"))) SPI_SWNSSAssert(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	//set NSS low
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_RESET);

	//change to GPIO mode
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	////GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
}

static void __attribute__((section(".itcmram"))) SPI_SWNSSDeassert(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	//set NSS high
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);

	//change back to HW NSS
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
}

int __attribute__((section(".itcmram"))) SPI_TransmitReceive(uint8_t *txBuf, uint8_t *rxBuf, uint32_t len)
{
	/* TODO:
	 * for > 64K we would need a loop - but it will not work!
	 * SCLK becomes floating, we will have wrong pulses!
	 * We do not loop here for 256 KB !
	 */
	int remLen;
#ifdef ENABLE_DCACHE
	int xLen = len;
#endif

	if (GDebug & DBG_SHOW_SPI)
	{
		//display SPI Tx (in bytes)
		int i;
		print_log(UART_OUT, "*D: SPI Tx:\r\n");
		for (i = 0; i < len; i++)
			print_log(UART_OUT, "%02x ", *(txBuf + i));
	}

	/* works - comes in smooth continuing bits */
	if (osSemaphoreAcquire(xSemaphoreSPI2, portMAX_DELAY) == osOK)
	{
#ifdef ENABLE_DCACHE
		SCB_CleanDCache_by_Addr((uint32_t *)txBuf, xLen);
		//SCB_CleanDCache();
#endif

		if (len > SPI_TOTAL_LEN)
		{
			remLen = len - SPI_TOTAL_LEN;
			len = SPI_TOTAL_LEN;
		}
		else
			remLen = 0;

		if ((GSysCfg.SPIcfg & CFG_SPI_CFG_HWNSS) >> 12)
		{
			//HW NSS
			if (remLen)
				SPI_SWNSSAssert();
		}
		else
		{
			//SW NSS: set NSS low
			/* ATT: this is too early! with SPI mode 0, clock starts low:
			 * we have a falling edge on SCLK!
			 * we have placed this code into HAL driver!
			 */
			////HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_RESET);
		}

		HAL_SPI_TransmitReceive_DMA(&hspi2, txBuf, rxBuf, len);
		/* TODO: this should be a loop! */
		if (remLen)
		{
			osSemaphoreAcquire(xSemaphoreSPI2Rx, portMAX_DELAY);
			HAL_SPI_TransmitReceive_DMA(&hspi2, txBuf + SPI_TOTAL_LEN, rxBuf + SPI_TOTAL_LEN, (uint16_t)remLen);
		}

		/* option: we could do when DMA is still running - but too early? */
#ifdef ENABLE_DCACHE
		SCB_InvalidateDCache_by_Addr((uint32_t *)rxBuf, xLen);
		//SCB_InvalidateDCache();
#endif

		/* wait for the completion of SPI - we can invalidate cache during DMA running - saving time */
		osSemaphoreAcquire(xSemaphoreSPI2Rx, portMAX_DELAY);

		if ((GSysCfg.SPIcfg & CFG_SPI_CFG_HWNSS) >> 12)
		{
			//HW NSS
			if (remLen)
				SPI_SWNSSDeassert();
		}
		else
			//SW NSS: set NSS high
			HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);

		osSemaphoreRelease(xSemaphoreSPI2);
	}

	if (GDebug & DBG_SHOW_SPI)
	{
		//display SPI Rx (in bytes)
		int i;
		print_log(UART_OUT, "\r\n*D: SPI Rx:\r\n");
		for (i = 0; i < len; i++)
			print_log(UART_OUT, "%02x ", *(rxBuf + i));
		print_log(UART_OUT, "\r\n");
	}

	return HAL_OK;
}

#if 0
void /*__attribute__((section(".itcmram")))*/ HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
}

void /*__attribute__((section(".itcmram")))*/ HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
}
#endif

void __attribute__((section(".itcmram"))) HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if(hspi->Instance == SPI2)
	{
		//XXXX
		/* this is too late to deassert SW NSS */
		////if ( ! ((GSysCfg.SPIcfg & CFG_SPI_CFG_HWNSS) >> 12))
		////	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);
		osSemaphoreRelease(xSemaphoreSPI2Rx);
	}
}

/* ********************************************************************************************************************
 *
 * SPI clock configuration - the same for both master SPI devices
 *
 * Assumption:
 * PLL2 is used for both SPI masters (and SPI slave devices)
 * 25 MHz XTAL => PLL2M = 4 ==> 2 MHz PLL ref_clk +. not modified
 * PLL2P = 4, PLL2Q = 4 => not modified
 * we configure PLL2N (=> DIVN2) and SPI BaudRatePrescaler
 * Remarks:
 * - changing DIVN2 needs to stop PLL2
 * - changing BaudRatePrescaler needs to stop SPI (done via config call for SPI)
 * - the DIVN2 value must be in range 96..418, for VCOH and PLL freq. kept in range 192..836 MHz
 * So, we have this configuration table:
 *
 * DIVM2 = 12, DIVP2 = 4, 25 MHz XTAL
 * SPI_CLK  /  BaudRatePrescaler			v_default
 * DIVN2 range:	2			4				8				16				32			64			128			256			512 not possible
 * ___________________________________________________________________________________________________________________________________
 * DIVN2 nom.                               96
 * nominal                                  50/8 = 6.250 MHz
 * DIVN2 min.                               72
 * min.			MHz		   	MHz				37.5/8 MHz		MHz				MHz			KHz     	292 KHz     146 KHz     73 KHz
 * DIVN2 max.                               460
 * max.			MHz	   		MHz				39.583333/8		MHz				MHz		    KHz			KHz			KHz			KHz
 *                                          29947.916
 *                                          29948 KHz
 * steps		KHz		 	KHz				520.833 KHz		KHz				KHz			KHz			KHz			KHz			KHz
 * ___________________________________________________________________________________________________________________________________
 *
 * based on this table:
 * - find in which range is the requested clock frequency: find best fitting range
 * - take the index in table (column) for the SPI BaudRatePrescaler and re-configure SPI
 *   (just needed for the SPI master devices, slave will follow and does not need)
 * - divide the requested clock frequency with the steps in column when found -> valid DIVN2 value
 * - reconfigure DIVN2 value in PLL2DIVR register, let all other bits there unchanged, disable and
 *   enable PLL
 * - reject invalid values (not in min-max table)
 * - a clock frequency 0 is assumed as reading back the DIVN2 value and return current frequency
 * - bear in mind that we use DIVN2 w/o this +/- 1 in the register value (set register with -1U)
 */
#define PLL_DIVM2		12
#define PLL_DIVN2		96
#define PLL_DIVP2		 4

static void SPI_PLLConfig(int DIVN2)
{
	uint32_t regVal, regVal2;
	int i;

	/* we have to disable PLL2 first */
	regVal2 = READ_REG(RCC->CR);
	regVal2 &= ~(1U << 26);
	WRITE_REG(RCC->CR, regVal2);

	/* do we have to wait for PLL2RDY = 0? */
	for (i = 0; i < 1000; i++)
		__NOP();

	regVal = READ_REG(RCC->PLL2DIVR);
	regVal &= ~RCC_PLL2DIVR_N2;
	regVal = regVal | (uint32_t)(((uint32_t)DIVN2 - 1U) & RCC_PLL2DIVR_N2);
	WRITE_REG(RCC->PLL2DIVR , regVal);
	for (i = 0; i < 1000; i++)
		__NOP();

	/* enable PLL2 again */
	regVal2 |= (1U << 26);
	WRITE_REG(RCC->CR, regVal2);
	for (i = 0; i < 2000; i++)
		__NOP();
}

int SPI_SetConfig(int khzFreq)
{
	int spiDivider = 2;
	uint32_t N2, P2, regVal;
	unsigned long xKHz = (unsigned long)khzFreq;

	regVal = READ_REG(RCC->PLL2DIVR);
	P2 = regVal & RCC_PLL2DIVR_P2;
	P2 >>= RCC_PLL2DIVR_P2_Pos;
	P2++;

	/* check the max. and min. possible frequency */
	/*
	 * DIVN2 min is 96 for VCOH
	 * DIVN2 max is 418 for VCOH
	 * DIVN2 can be: 4..512 (0x1FF + 1)
	 */
	if (khzFreq < 74)						/* spiclk < 74 [KHz] fails! */
		khzFreq = 75;
	if (khzFreq > (4*29948))
		khzFreq = (4*29948);
	/* ATTENTION: on scope - it looks like > 90 MHz does not work anymore:
	 * even there are signals - but the NSS signal toggles, goes high in between (a scope problem or too fast for SPI device?)
	 * Avoid to use > 90 MHz even possible to configure here: (< 130 MHz is theoretical limit)
	 */

	/* calculate the needed SPI divider */
	if (khzFreq <= (29948 / 32))
		spiDivider = 256;
	if (khzFreq <= (29948 / 16))
		spiDivider = 128;
	else if (khzFreq <= (29948 / 8))
		spiDivider = 64;
	else if (khzFreq <= (29948 / 4))
		spiDivider = 32;
	else if (khzFreq <= (29948 / 2))
		spiDivider = 16;
	else if (khzFreq <=29948)
		spiDivider = 8;
	else if (khzFreq <= (2*29948))
		spiDivider = 4;

	/* calculate from DIVN2 */
	N2  = khzFreq * spiDivider * P2 * PLL_DIVM2;
	N2 /= 25000;	//we are still in KHz range
	N2++;

	/*
	 * ATT: we should actually make sure the SPIs are not in use!
	 * CRITICAL_SECTION
	 */
	if (osSemaphoreAcquire(xSemaphoreSPI2, portMAX_DELAY /*1000*/) == osOK)
	{
		SPI_PLLConfig(N2);			//set the PLL divider - affects all SPIs

		/* if the SPI divider is different as the default - we had to reconfigure the SPI master devices */
		if (spiDivider != sSPIdivider)
		{
			sSPIdivider = spiDivider;
			MX_SPI2_Init();
		}

		/* we set the value provided: we could also read back PLL setting (effective) and set this one */
		SYSCFG_setCfg(2, xKHz);

		osSemaphoreRelease(xSemaphoreSPI2);
		return 1;
	}
	/* END CRITICAL_SECTION */
	return 0;			//error - not done
}

int SPI_GetClock(void)
{
	uint32_t N2, P2, regVal;

	regVal = READ_REG(RCC->PLL2DIVR);
	N2 = regVal & RCC_PLL2DIVR_N2;
	N2 >>= RCC_PLL2DIVR_N2_Pos;
	N2++;
	P2 = regVal & RCC_PLL2DIVR_P2;
	P2 >>= RCC_PLL2DIVR_P2_Pos;
	P2++;

	regVal = 25000000 / PLL_DIVM2;		//XTAL divided by M2
	regVal *= N2;
	regVal /= P2;
	regVal /= sSPIdivider;

	/* it is only correct when 25 MHz XTAL clock is assumed */
	return (int)((regVal + 500) / 1000);
}
