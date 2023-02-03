/**
 * bsp_driver_sd.c
 *
 *  Created on: May 22, 2018
 *  Author: Torsten Jaekel
 */

#include "bsp_driver_sd.h"

SD_HandleTypeDef uSdHandle;

uint8_t BSP_SD_Init(void)
{
  uint8_t sd_state = MSD_OK;

  /* uSD device interface configuration */
  uSdHandle.Instance = SDMMC2;

  /* if CLKDIV = 0 then SDMMC Clock frequency = SDMMC Kernel Clock
     else SDMMC Clock frequency = SDMMC Kernel Clock / [2 * CLKDIV].
  */
  /* Code for high performance */
  uSdHandle.Init.ClockDiv            = 50;
  uSdHandle.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  uSdHandle.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
  uSdHandle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
  uSdHandle.Init.BusWide             = SDMMC_BUS_WIDE_4B;

  /* Msp SD initialization */
  BSP_SD_MspInit(&uSdHandle, NULL);

  /* Check if SD card is present */
  if (BSP_SD_IsDetected() != SD_PRESENT)
  {
    BSP_SD_MspDeInit(&uSdHandle, NULL);
    return MSD_ERROR_SD_NOT_PRESENT;
  }

  /* HAL SD initialization */
  if (HAL_SD_Init(&uSdHandle) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return  sd_state;
}

/**
    @brief  DeInitializes the SD card device.
    @retval SD status
*/
uint8_t BSP_SD_DeInit(void)
{
  uint8_t sd_state = MSD_OK;

  uSdHandle.Instance = SDMMC2;

  /* HAL SD deinitialization */
  if (HAL_SD_DeInit(&uSdHandle) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  /* Msp SD deinitialization */
  uSdHandle.Instance = SDMMC2;
  BSP_SD_MspDeInit(&uSdHandle, NULL);

  return  sd_state;
}

/**
    @brief  Configures Interrupt mode for SD1 detection pin.
    @retval Returns 0
*/
uint8_t BSP_SD_ITConfig(void)
{
  return MSD_OK;
}

/**
   @brief  Detects if SD card is correctly plugged in the memory slot or not.
   @retval Returns if SD is detected or not
*/
uint8_t BSP_SD_IsDetected(void)
{
  __IO uint8_t status = SD_PRESENT;


  return status;
}

/**
    @brief  Reads block(s) from a specified address in an SD card, in polling mode.
    @param  pData: Pointer to the buffer that will contain the data to transmit
    @param  ReadAddr: Address from where data is to be read
    @param  NumOfBlocks: Number of SD blocks to read
    @param  Timeout: Timeout for read operation
    @retval SD status
*/
uint8_t BSP_SD_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{

  if ( HAL_SD_ReadBlocks(&uSdHandle, (uint8_t *)pData, ReadAddr, NumOfBlocks, Timeout) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }

}

/**
    @brief  Writes block(s) to a specified address in an SD card, in polling mode.
    @param  pData: Pointer to the buffer that will contain the data to transmit
    @param  WriteAddr: Address from where data is to be written
    @param  NumOfBlocks: Number of SD blocks to write
    @param  Timeout: Timeout for write operation
    @retval SD status
*/
uint8_t BSP_SD_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout)
{

  if ( HAL_SD_WriteBlocks(&uSdHandle, (uint8_t *)pData, WriteAddr, NumOfBlocks, Timeout) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
    @brief  Reads block(s) from a specified address in an SD card, in DMA mode.
    @param  pData: Pointer to the buffer that will contain the data to transmit
    @param  ReadAddr: Address from where data is to be read
    @param  NumOfBlocks: Number of SD blocks to read
    @retval SD status
*/
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{
  if ( HAL_SD_ReadBlocks_DMA(&uSdHandle, (uint8_t *)pData, ReadAddr, NumOfBlocks) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
    @brief  Writes block(s) to a specified address in an SD card, in DMA mode.
    @param  pData: Pointer to the buffer that will contain the data to transmit
    @param  WriteAddr: Address from where data is to be written
    @param  NumOfBlocks: Number of SD blocks to write
    @retval SD status
*/
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks)
{

  if ( HAL_SD_WriteBlocks_DMA(&uSdHandle, (uint8_t *)pData, WriteAddr, NumOfBlocks) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }

}

/**
    @brief  Erases the specified memory area of the given SD card.
    @param  StartAddr: Start byte address
    @param  EndAddr: End byte address
    @retval SD status
*/
uint8_t BSP_SD_Erase(uint32_t StartAddr, uint32_t EndAddr)
{

  if ( HAL_SD_Erase(&uSdHandle, StartAddr, EndAddr) == HAL_OK)
  {
    return MSD_OK;
  }
  else
  {
    return MSD_ERROR;
  }
}

/**
    @brief  Initializes the SD MSP.
    @param  hsd SD handle
    @param  Params User parameters
    @retval None
*/
/*__weak*/ void BSP_SD_MspInit(SD_HandleTypeDef *hsd, void *Params)
{
  /* __weak function can be modified by the application */

  GPIO_InitTypeDef gpio_init_structure;

  /* SD pins are in conflict with Camera pins on the Disco board
     therefore Camera must be power down before using the BSP SD
     To power down the camera , Set GPIOJ pin 14 to high
  */

  /* Enable GPIO J clock */
  __HAL_RCC_GPIOG_CLK_ENABLE();

  gpio_init_structure.Pin       = GPIO_PIN_3;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull      = GPIO_NOPULL;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOG, &gpio_init_structure);

  /* Set the camera POWER_DOWN pin (active high) */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_RESET);

  /* Enable SDIO clock */
  __HAL_RCC_SDMMC2_CLK_ENABLE();

  /* Enable GPIOs clock */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();


  /* Common GPIO configuration */
  gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
  gpio_init_structure.Pull      = GPIO_PULLUP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio_init_structure.Alternate = GPIO_AF9_SDIO2;

  /* SDMMC GPIO CLKIN PB8, D0 PC8, D1 PC9, D2 PC10, D3 PC11, CK PC12, CMD PD2 */
  /* GPIOC configuration */
  gpio_init_structure.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_14 | GPIO_PIN_15;

  HAL_GPIO_Init(GPIOB, &gpio_init_structure);

  /* GPIOD configuration */
  gpio_init_structure.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  gpio_init_structure.Alternate = GPIO_AF11_SDIO2;
  HAL_GPIO_Init(GPIOD, &gpio_init_structure);

  /* NVIC configuration for SDIO interrupts */
  HAL_NVIC_SetPriority(SDMMC2_IRQn, 12, 0);
  HAL_NVIC_EnableIRQ(SDMMC2_IRQn);
}

/**
    @brief  DeInitializes the SD MSP.
    @param  hsd SD handle
    @param  Params User parameters
    @retval None
*/
/*__weak*/ void BSP_SD_MspDeInit(SD_HandleTypeDef *hsd, void *Params)
{
  /* Disable NVIC for SDIO interrupts */
  HAL_NVIC_DisableIRQ(SDMMC2_IRQn);

  /* DeInit GPIO pins can be done in the application
     (by surcharging this __weak function) */

  /* Disable SDMMC1 clock */
  __HAL_RCC_SDMMC2_CLK_DISABLE();

  /* GPIO pins clock and DMA clocks can be shut down in the application
     by surcharging this __weak function */
}

/**
    @brief  Handles SD card interrupt request.
    @retval None
*/
void BSP_SD_IRQHandler(void)
{
  HAL_SD_IRQHandler(&uSdHandle);
}

/**
    @brief  Gets the current SD card data status.
    @retval Data transfer state.
             This value can be one of the following values:
               @arg  SD_TRANSFER_OK: No data transfer is acting
               @arg  SD_TRANSFER_BUSY: Data transfer is acting
               @arg  SD_TRANSFER_ERROR: Data transfer error
*/
uint8_t BSP_SD_GetCardState(void)
{
  return ((HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

/**
    @brief  Get SD information about specific SD card.
    @param  CardInfo: Pointer to HAL_SD_CardInfoTypedef structure
    @retval None
*/
void BSP_SD_GetCardInfo(BSP_SD_CardInfo *CardInfo)
{
  HAL_SD_GetCardInfo(&uSdHandle, CardInfo);
}

/**
    @brief SD Abort callbacks
    @param hsd SD handle
    @retval None
*/
void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_AbortCallback();
}


/**
    @brief Tx Transfer completed callbacks
    @param hsd SD handle
    @retval None
*/
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_WriteCpltCallback();
}

/**
    @brief Rx Transfer completed callbacks
    @param hsd SD handle
    @retval None
*/
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  BSP_SD_ReadCpltCallback();
}

/**
    @brief BSP SD Abort callbacks
    @retval None
*/
__weak void BSP_SD_AbortCallback(void)
{

}

/**
    @brief BSP Tx Transfer completed callbacks
    @retval None
*/
__weak void BSP_SD_WriteCpltCallback(void)
{

}

/**
    @brief BSP Rx Transfer completed callbacks
    @retval None
*/
__weak void BSP_SD_ReadCpltCallback(void)
{

}

void SDMMC2_IRQHandler(void)
{
  BSP_SD_IRQHandler();
}
