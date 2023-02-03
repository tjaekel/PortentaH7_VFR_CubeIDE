/**
 * sd_diskio.c
 *
 *  Created on: May 22, 2018
 *  Author: Torsten Jaekel
 */

#include "ff_gen_drv.h"
#include "sd_diskio.h"

#define QUEUE_SIZE         (sizeof(osEvent))	//(uint32_t) 10
#define READ_CPLT_MSG      (uint32_t) 1
#define WRITE_CPLT_MSG     (uint32_t) 2
/*
 * the following Timeout is useful to give the control back to the applications
 * in case of errors in either BSP_SD_ReadCpltCallback() or BSP_SD_WriteCpltCallback()
 * the value by default is as defined in the BSP platform driver otherwise 30 secs
 */
#define SD_TIMEOUT 5 * 1000
#define SD_DEFAULT_BLOCK_SIZE 512

/*
 * Depending on the use case, the SD card initialization could be done at the
 * application level: if it is the case define the flag below to disable
 * the BSP_SD_Init() call in the SD_Initialize() and add a call to 
 * BSP_SD_Init() elsewhere in the application.
 */

/* #define DISABLE_SD_INIT */

/* 
 * when using cachable memory region, it may be needed to maintain the cache
 * validity. Enable the define below to activate a cache maintenance at each
 * read and write operation.
 * Notice: This is applicable only for cortex M7 based platform.
 */

/* #define ENABLE_SD_DMA_CACHE_MAINTENANCE  1 */

/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

static osMessageQueueId_t *SDQueueID;

static DSTATUS SD_CheckStatus(BYTE lun);
DSTATUS SD_initialize (BYTE);
DSTATUS SD_status (BYTE);
DRESULT SD_read (BYTE, BYTE*, DWORD, UINT);
#if _USE_WRITE == 1
DRESULT SD_write (BYTE, const BYTE*, DWORD, UINT);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
DRESULT SD_ioctl (BYTE, BYTE, void*);
#endif  /* _USE_IOCTL == 1 */

const Diskio_drvTypeDef  SD_Driver =
{
  SD_initialize,
  SD_status,
  SD_read,
#if  _USE_WRITE == 1
  SD_write,
#endif /* _USE_WRITE == 1 */

#if  _USE_IOCTL == 1
  SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};

static DSTATUS SD_CheckStatus(BYTE lun)
{
  (void)lun;

  Stat = STA_NOINIT;

  if(BSP_SD_GetCardState() == MSD_OK)
  {
    Stat &= (DSTATUS)~STA_NOINIT;
  }

  return Stat;
}

/**
  * @brief  Initializes a Drive
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_initialize(BYTE lun)
{
  Stat = STA_NOINIT;
  uint8_t err;

  /*
   * check that the kernel has been started before continuing
   * as the osMessage API will fail otherwise
   */
  if(/*osKernelRunning()*/ 1)
  {
#if !defined(DISABLE_SD_INIT)

    err = BSP_SD_Init();
    if (err == MSD_OK)
    {
      Stat = SD_CheckStatus(lun);
    }

#else
    Stat = SD_CheckStatus(lun);
#endif

    /*
     * if the SD is correctly initialized, create the operation queue
     */
    if (Stat != STA_NOINIT)
    {
    	SDQueueID = osMessageQueueNew(1, QUEUE_SIZE, NULL);
    }
  }
  return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS SD_status(BYTE lun)
{
  return SD_CheckStatus(lun);
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT SD_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  (void)lun;

  DRESULT res = RES_ERROR;
  osEvent event;
  uint32_t timer;
  uint8_t msgPrio = 0;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
  uint32_t alignedAddr;
#endif

  if(BSP_SD_ReadBlocks_DMA((uint32_t*)buff,
                           (uint32_t) (sector),
                           count) == MSD_OK)
  {
    /* wait for a message from the queue or a timeout */
	if (osMessageQueueGet(SDQueueID, &event, &msgPrio, SD_TIMEOUT) != osOK)
		return res;

    if (event.status == osEventMessage)
    {
      if (event.value.v == READ_CPLT_MSG)
      {
        timer = osKernelSysTick() + SD_TIMEOUT;
        /* block until SDIO IP is ready or a timeout occur */
        while(timer > osKernelSysTick())
        {
          if (BSP_SD_GetCardState() == SD_TRANSFER_OK)
          {
            res = RES_OK;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
            /*
               the SCB_InvalidateDCache_by_Addr() requires a 32-Byte aligned address,
               adjust the address and the D-Cache size to invalidate accordingly.
             */
            alignedAddr = (uint32_t)buff & (uint32_t)~0x1F;
            SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, (int32_t)(count*BLOCKSIZE + ((uint32_t)buff - alignedAddr)));
#endif
            break;
          }
        }
      }
    }
  }

  return res;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT SD_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
  (void)lun;

  DRESULT res = RES_ERROR;
  osEvent event;
  uint32_t timer;
  uint8_t msgPrio = 0;
#if (ENABLE_SD_DMA_CACHE_MAINTENANCE == 1)
  uint32_t alignedAddr;
  /*
   the SCB_CleanDCache_by_Addr() requires a 32-Byte aligned address
   adjust the address and the D-Cache size to clean accordingly.
   */
  alignedAddr = (uint32_t)buff & (uint32_t)~0x1F;
  SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, (int32_t)(count*BLOCKSIZE + ((uint32_t)buff - alignedAddr)));
#endif

  if(BSP_SD_WriteBlocks_DMA((uint32_t*)buff,
                            (uint32_t) (sector),
                            count) == MSD_OK)
  {
    /* Get the message from the queue */
    osMessageQueueGet(SDQueueID, &event, &msgPrio, SD_TIMEOUT);

    if (event.status == osEventMessage)
    {
      if (event.value.v == WRITE_CPLT_MSG)
      {
        timer = osKernelSysTick() + SD_TIMEOUT;
        /* block until SDIO IP is ready or a timeout occur */
        while(timer > osKernelSysTick())
        {
          if (BSP_SD_GetCardState() == SD_TRANSFER_OK)
          {
            res = RES_OK;
            break;
          }
        }
      }
    }
  }

  return res;
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT SD_ioctl(BYTE lun, BYTE cmd, void *buff)
{
  (void)lun;

  DRESULT res = RES_ERROR;
  BSP_SD_CardInfo CardInfo;

  if (Stat & STA_NOINIT) return RES_NOTRDY;

  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;

  /* Get number of sectors on the disk (DWORD) */
  case GET_SECTOR_COUNT :
    BSP_SD_GetCardInfo(&CardInfo);
    *(DWORD*)buff = CardInfo.LogBlockNbr;
    res = RES_OK;
    break;

  /* Get R/W sector size (WORD) */
  case GET_SECTOR_SIZE :
    BSP_SD_GetCardInfo(&CardInfo);
    *(WORD*)buff = (WORD)CardInfo.LogBlockSize;
    res = RES_OK;
    break;

  /* Get erase block size in unit of sector (DWORD) */
  case GET_BLOCK_SIZE :
    BSP_SD_GetCardInfo(&CardInfo);
    *(DWORD*)buff = CardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
    res = RES_OK;
    break;

  default:
    res = RES_PARERR;
  }

  return res;
}
#endif /* _USE_IOCTL == 1 */

/**
  * @brief Tx Transfer completed callbacks
  * @param hsd: SD handle
  * @retval None
  */

//void BSP_SD_WriteCpltCallback(uint32_t SdCard)
void BSP_SD_WriteCpltCallback(void)
{
	osEvent event;
	event.value.v = WRITE_CPLT_MSG;
	event.status = osEventMessage;
	osMessageQueuePut(SDQueueID, &event, osPriorityNormal, 0);
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsd: SD handle
  * @retval None
  */

//void BSP_SD_ReadCpltCallback(uint32_t SdCard)
void BSP_SD_ReadCpltCallback(void)
{
	osEvent event;
	event.value.v = READ_CPLT_MSG;
	event.status = osEventMessage;
	osMessageQueuePut(SDQueueID, &event, osPriorityNormal, 0);
}

#if 0
/**
  * @brief Tx Transfer completed callbacks
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
  (void)hsd;

  BSP_SD_WriteCpltCallback();
}

/**
  * @brief Rx Transfer completed callbacks
  * @param hsd: SD handle
  * @retval None
  */
void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
  (void)hsd;

  BSP_SD_ReadCpltCallback();
}
#endif
