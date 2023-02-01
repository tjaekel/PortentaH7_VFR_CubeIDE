/*
 * usbd_main.c
 *
 *  Created on: Mar 14, 2016
 *  Author: Torsten Jaekel
 */

#include "main.h"

#include "usbd_main.h"
#include "usbd_desc_audio.h"
#include "usbd_audio_if.h"

#include "SDCard.h"
#include "usbd_desc.h"
#include "usbd_storage.h"

//#ifdef WITH_USB_AUDIO
extern void USBD_AUDIO_SetMode(int mode);
//#endif

static void USB_InterfaceDeinit(void);

USBD_HandleTypeDef USBD_Device2;
extern USBD_CDC_ItfTypeDef USBD_CDC_fops2;

int gUSBInterface = 0;
int gUSBrunning;
int gUSBMode = 0;

////extern USBD_HandleTypeDef USBD_Device;

void USB_Interface(int mode)
{
	//clear the USB buffer

	if (mode)
	{
		if ( ! gUSBInterface)
		{
			gUSBMode = mode;
			////USBD_DeInit(&USBD_Device);

//#ifdef WITH_USB_AUDIO
			USBD_AUDIO_SetMode(mode);
//#endif

			HAL_PWREx_EnableUSBVoltageDetector();

//#ifdef WITH_USB_AUDIO
			//mode 3 = USB Audio Out (from PC)
			//mode 1 = USB Audio In (to PC)
#if 1
			if (mode == 1)
			{
				/* Init Device Library */
				USBD_Init(&USBD_Device2, &AUDIO_Desc, 0);

				/* Add Supported Class */
				USBD_RegisterClass(&USBD_Device2, USBD_AUDIO_CLASS_IN);

				/* Add Interface call backs for AUDIO Class */
				USBD_AUDIO_RegisterInterface(&USBD_Device2, &USBD_AUDIO_fops);

				/* Start Device Process */
				USBD_Start(&USBD_Device2);

				gUSBInterface = 1;
			}
			else
#endif
//#endif
#if 0
			if (mode == 2)
			{
				if (SDCard_GetStatus() != 0)
				{
					/* just allowed if SDMMC is initialized */

					/* use USB as MSC device to manage files on SD Card */
					USBD_Init(&USBD_Device2, &MSC_Desc, 0);

					/* Add Supported Class */
					USBD_RegisterClass(&USBD_Device2, USBD_MSC_CLASS);

					/* Add Storage callbacks for MSC Class */
					USBD_MSC_RegisterStorage(&USBD_Device2, &USBD_DISK_fops);

					/* Start Device Process */
					USBD_Start(&USBD_Device2);

					gUSBInterface = 1;
				}
			}
			else
#endif
//#ifdef WITH_USB_AUDIO
			if (mode == 3)
			{
				/* use USB as Audio Out from PC */
				/* Init Device Library */
				USBD_Init(&USBD_Device2, &AUDIO_Desc, 0);

				/* Add Supported Class */
				USBD_RegisterClass(&USBD_Device2, USBD_AUDIO_CLASS_OUT);

				/* Add Interface call backs for AUDIO Class */
				USBD_AUDIO_RegisterInterface(&USBD_Device2, &USBD_AUDIO_fops);

				/* Start Device Process */
				USBD_Start(&USBD_Device2);

				gUSBInterface = 1;
			}
			else
			if (mode == 4)
			{
				//VCP UART (CDC)
				extern USBD_DescriptorsTypeDef CDC_Desc;
				USBD_Init(&USBD_Device2, &CDC_Desc, 0);
				USBD_RegisterClass(&USBD_Device2, USBD_CDC_CLASS);
				USBD_CDC_RegisterInterface(&USBD_Device2, &USBD_CDC_fops2);
				USBD_Start(&USBD_Device2);
			}
//#endif
		}
	}
	else
		USB_InterfaceDeinit();
}

static void USB_InterfaceDeinit(void)
{
	if (gUSBInterface)
	{
		gUSBInterface = 0;

		USBD_Stop(&USBD_Device2);

		USBD_DeInit(&USBD_Device2);

		////HAL_PWREx_DisableUSBVoltageDetector();
	}
}

void USB_ClearBuffers(void)
{
//#ifdef WITH_USB_AUDIO
	////USBD_AUDIO_ClearBuffer();
//#endif
}
