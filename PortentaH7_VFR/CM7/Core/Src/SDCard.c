/**
 * SDCard.c
 *
 *  Created on: May 22, 2018
 *  Author: Torsten Jaekel
 */

#include "SDCard.h"
#include "VCP_UART.h"				/* for print_log() */

FATFS SDFatFs;  /* File system object for SD card logical drive */
char SDPath[4] = {'/', 0}; /* SD card logical drive path */
static uint8_t sSDCardOpen = 0;
static uint8_t sSDCardLinked = 0;

#if 0
static uint8_t workBuffer[_MAX_SS];
#endif

static void SDCard_Deinit(void);

#ifdef TEST_SD_CARD

FIL MyFile;     /* File object */

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int SDCard_Test(void)
{
  FRESULT res;                                          /* FatFs function common result code */
  uint32_t byteswritten, bytesread;                     /* File write/read counts */
  static uint8_t wtext[] = "If you see this: this is STM32 working with FatFs"; /* File write buffer */
  static uint8_t rtext[100];             				/* File read buffer */

  if(sSDCardOpen)
  {
#if 1
	  if(f_open(&MyFile, "TEST.TXT", FA_READ) != FR_OK)
	  {
	       /* 'TEST.TXT' file Open for read Error */
	       ; //return 5;			//error
	  }
	  else
	  {
	      res = f_read(&MyFile, rtext, sizeof(rtext), (UINT*)&bytesread);

	      if((bytesread == 0) || (res != FR_OK))
	      {
	          	/* 'TEST.TXT' file Read or EOF Error */
	    	  	SDCard_Deinit();
	          	return 6;		//error
	      }
	      else
	      {
	          	f_close(&MyFile);
	      }
	  }
#endif

      if(f_open(&MyFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
      {
        	/* 'STM32.TXT' file Open for write Error */
    	  SDCard_Deinit();
        	return 3;			//error
      }
      else
      {
        	res = f_write(&MyFile, wtext, sizeof(wtext), (void *)&byteswritten);

        	if((byteswritten == 0) || (res != FR_OK))
        	{
        		/* 'STM32.TXT' file Write or EOF Error */
        		SDCard_Deinit();
        		return 4;			//error
        	}
        	else
        	{
        		f_close(&MyFile);

        		if(f_open(&MyFile, "STM32.TXT", FA_READ) != FR_OK)
        		{
        			/* 'STM32.TXT' file Open for read Error */
        			SDCard_Deinit();
        			return 5;			//error
        		}
        		else
        		{
        			res = f_read(&MyFile, rtext, sizeof(rtext), (UINT*)&bytesread);

        			if((bytesread == 0) || (res != FR_OK))
        			{
        				/* 'STM32.TXT' file Read or EOF Error */
        				SDCard_Deinit();
        				return 6;		//error
        			}
        			else
        			{
        				f_close(&MyFile);

        				if((bytesread != byteswritten))
        				{
        					/* Read data is different from the expected data */
        					SDCard_Deinit();
        					return 7;		//error
        				}
        				else
        				{
        					/* Success of the demo: no error occurrence */
        					/* keep SDCard open */
        					return 0;		//no error
        				}
        			}
        		}
        	}
      }
  }

  SDCard_Deinit();
  return 7;
}
#endif

int SDCard_Init(int mode)
{
	if (mode > 0)
	{
		if (sSDCardOpen == 0)
		{
			if (BSP_SD_IsDetected() == SD_NOT_PRESENT)
				return 0;

			if(FATFS_LinkDriver(&SD_Driver, SDPath) == 0)
			{
				sSDCardLinked = 1;

#if 1
				/* with format SD Card - condition: must be closed, via sdinit 0 before ! */
				if (mode == 2)				//format the SD Card
				{
					uint8_t *workBuffer = MEM_PoolAlloc(MEM_POOL_SEG_SIZE);	//at least: _MAX_SS
					if ( ! workBuffer)
					{
						return 4;			//Out of Memory
					}

					/* WARNING: Formatting the SD card will delete all content on the device */
					if (f_mkfs(SDPath, FM_ANY, 0, workBuffer, MEM_POOL_SEG_SIZE) != FR_OK)
					{
						/* FatFs Format Error */
						MEM_PoolFree(workBuffer);
						return 3;			//error
					}
					MEM_PoolFree(workBuffer);
				}
#endif

				if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
					/* FatFs Initialization Error */
					return 2;				//error
			}

			sSDCardOpen = 1;
		}
	}
	else
		SDCard_Deinit();

	return 0;
}

static void SDCard_Deinit(void)
{
	if (sSDCardOpen)
	{
		f_mount(NULL, (TCHAR const*)SDPath, 0);
		sSDCardOpen = 0;
	}
	if (sSDCardLinked)
	{
		FATFS_UnLinkDriver(SDPath);
		sSDCardLinked = 0;
	}
}

inline int SDCard_GetStatus(void)
{
	if (sSDCardOpen)
		return 1;
	else
		return 0;
}

FRESULT SDCard_ScanFiles(char* path, EResultOut out)
{
    int num_of_files = 0;
    int num_of_dirs = 0;

    FRESULT res;
    static FILINFO fno;
    static DIR dir;

    if ( ! SDCard_GetStatus())
    	return FR_NOT_ENABLED;

    res = f_opendir(&dir, path);                       		/* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   	/* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  	/* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             	/* Ignore dot entry */
            if (fno.fattrib & AM_HID) continue;				/* Ignore Hidden (and SYS) */
            if (fno.fattrib & AM_DIR) {                    	/* It is a directory */
                num_of_dirs++;
                print_log(out, (const char *)"D: /%s\r\n", fno.fname);
            } else {                                       	/* It is a file. */
                print_log(out, (const char *)"F: %8ld %s/%-13s\r\n", fno.fsize, path, fno.fname);
                num_of_files++;
            }
        }
        f_closedir(&dir);
    }
    print_log(out, (const char *)"%d Dirs | %d Files\r\n", num_of_dirs, num_of_files);
    return res;
}
