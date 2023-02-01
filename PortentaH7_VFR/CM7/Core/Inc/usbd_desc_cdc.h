/**
 * usbd_desc_cdc.h
 *
 *  Created on: Mar 3, 2017
 *      Author: Torsten Jaekel
 */

#ifndef USBD_DESC_CDC_H_
#define USBD_DESC_CDC_H_

#include "usbd_def.h"

#if 1
//MCU internal addresses to read Device ID
//#define         DEVICE_ID1_CDC          (0x1FFF7A10)
//#define         DEVICE_ID2_CDC          (0x1FFF7A14)
//#define         DEVICE_ID3_CDC          (0x1FFF7A18)
//for Portenta H7 MCU it is:
#define         DEVICE_ID1_CDC          (0x1FF1E800)
#define         DEVICE_ID2_CDC          (0x1FF1E804)
#define         DEVICE_ID3_CDC          (0x1FF1E808)
#endif

#define  USB_SIZ_STRING_SERIAL_CDC      0x1A

extern USBD_DescriptorsTypeDef CDC_Desc;

#endif /* USBD_DESC_CDC_H_ */
