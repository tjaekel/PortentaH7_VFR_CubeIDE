/**
  ******************************************************************************
  * @file    SDCard.h
  * @author  Torsten Jaekel
  * @version V1.1.0
  * @date    04-September-2016
  * @brief   The SD Card WAV file interfaces
  ******************************************************************************
  */

#ifndef SDCARD_H_
#define SDCARD_H_

#include "main.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "bsp_driver_sd.h"

#include "MEM_Pool.h"
#include "VCP_UART.h"

extern int  SDCard_Init(int mode);
extern int  SDCard_GetStatus(void);

//needs SDCard_Init() first
#ifdef TEST_SD_CARD
extern int SDCard_Test(void);
#endif

FRESULT SDCard_ScanFiles(char* path, EResultOut out);

#endif /* SDCARD_H_ */
