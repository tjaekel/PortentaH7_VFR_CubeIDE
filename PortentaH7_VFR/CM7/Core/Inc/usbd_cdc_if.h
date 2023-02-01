/**
 * usbd_cdc_if.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef __USBD_CDC_IF_H
#define __USBD_CDC_IF_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "usbd_cdc.h"

 typedef struct {
	 int bitrate;
	 int format;
	 int paritytype;
	 int datatype;
	 int changed;
 } LINE_CODING;

extern USBD_CDC_ItfTypeDef  USBD_CDC_fops;
extern USBD_HandleTypeDef USBD_Device;
extern int gUSBInterface;

////uint8_t CDC_Transmit_HS(uint8_t* Buf, uint16_t Len);

uint16_t CDC_GetRx(uint8_t *Buf, uint16_t Len);
uint16_t CDC_PutTx(uint8_t *Buf, uint16_t Len);
int CDC_isReady(void);

#ifdef __cplusplus
}
#endif
  
#endif /* __USBD_CDC_IF_H */
