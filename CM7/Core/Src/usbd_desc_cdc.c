/**
 * usbd_desc_cdc.c
 *
 *  Created on: Mar 3, 2017
 *      Author: Torsten Jaekel
 */

#include "usbd_core.h"
#include "usbd_desc_cdc.h"
#include "usbd_conf_cdc.h"

//USE STM VID, PID for STM Virtual COM Port Driver: install file: VCP_V1.5.0_Setup_W7_x64_64bits.exe (Win7 version!)
//                                        STM       Arduino     - Arduino PID, VID does not find driver
#define USBD_VID_CDC                      0x0483	//0x2341	//0x0483	//keep unchanged for VCP Driver !
#define USBD_PID_CDC                      0x5740	//0x025B	//0x5740	//keep unchanged for VCP Driver !

#define USBD_LANGID_STRING_CDC            0x409
#define USBD_MANUFACTURER_STRING_CDC      "P7DualSPIder"
#define USBD_PRODUCT_HS_STRING_CDC        "VCP HS Mode"
#define USBD_PRODUCT_FS_STRING_CDC        "VCP FS Mode"
#define USBD_CONFIGURATION_HS_STRING_CDC  "CDC Config"
#define USBD_INTERFACE_HS_STRING_CDC      "CDC Interface"
#define USBD_CONFIGURATION_FS_STRING_CDC  "CDC Config"
#define USBD_INTERFACE_FS_STRING_CDC      "CDC Interface"

uint8_t *USBD_CDC_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_CDC_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_CDC_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_CDC_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_CDC_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_CDC_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_CDC_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
#ifdef USB_SUPPORT_USER_STRING_DESC
uint8_t *USBD_CDC_USRStringDesc(USBD_SpeedTypeDef speed, uint8_t idx, uint16_t *length);
#endif /* USB_SUPPORT_USER_STRING_DESC */

USBD_DescriptorsTypeDef CDC_Desc = {
  USBD_CDC_DeviceDescriptor,
  USBD_CDC_LangIDStrDescriptor,
  USBD_CDC_ManufacturerStrDescriptor,
  USBD_CDC_ProductStrDescriptor,
  USBD_CDC_SerialStrDescriptor,
  USBD_CDC_ConfigStrDescriptor,
  USBD_CDC_InterfaceStrDescriptor,
};

/* USB Standard Device Descriptor */
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4
#endif
__ALIGN_BEGIN uint8_t USBD_DeviceDesc_CDC[USB_LEN_DEV_DESC] __ALIGN_END = {
  0x12,                       /* bLength */
  USB_DESC_TYPE_DEVICE,       /* bDescriptorType */
  0x00,                       /* bcdUSB */
  0x02,
  0x00,                       /* bDeviceClass */
  0x00,                       /* bDeviceSubClass */
  0x00,                       /* bDeviceProtocol */
  USB_MAX_EP0_SIZE,           /* bMaxPacketSize */
  LOBYTE(USBD_VID_CDC),       /* idVendor */
  HIBYTE(USBD_VID_CDC),       /* idVendor */
  LOBYTE(USBD_PID_CDC),       /* idVendor */
  HIBYTE(USBD_PID_CDC),       /* idVendor */
  0x00,                       /* bcdDevice rel. 2.00 */
  0x02,
  USBD_IDX_MFC_STR,           /* Index of manufacturer string */
  USBD_IDX_PRODUCT_STR,       /* Index of product string */
  USBD_IDX_SERIAL_STR,        /* Index of serial number string */
  USBD_MAX_NUM_CONFIGURATION  /* bNumConfigurations */
}; /* USB_DeviceDescriptor */

/* USB Standard Device Descriptor */
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4
#endif
__ALIGN_BEGIN uint8_t USBD_LangIDDesc_CDC[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
  USB_LEN_LANGID_STR_DESC,
  USB_DESC_TYPE_STRING,
  LOBYTE(USBD_LANGID_STRING_CDC),
  HIBYTE(USBD_LANGID_STRING_CDC),
};

uint8_t USBD_StringSerial_CDC[USB_SIZ_STRING_SERIAL_CDC] =
{
  USB_SIZ_STRING_SERIAL_CDC,
  USB_DESC_TYPE_STRING,
};


#if defined ( __ICCARM__ ) /*!< IAR Compiler */
  #pragma data_alignment=4
#endif
__ALIGN_BEGIN uint8_t USBD_StrDesc_CDC[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);
static void Get_SerialNum(void);

/**
  * @brief  Returns the device descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_CDC_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(USBD_DeviceDesc_CDC);
  return (uint8_t*)USBD_DeviceDesc_CDC;
}

/**
  * @brief  Returns the LangID string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_CDC_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = sizeof(USBD_LangIDDesc_CDC);
  return (uint8_t*)USBD_LangIDDesc_CDC;
}

/**
  * @brief  Returns the product string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_CDC_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  if(speed == USBD_SPEED_HIGH)
  {
    USBD_GetString((uint8_t *)USBD_PRODUCT_HS_STRING_CDC, USBD_StrDesc_CDC, length);
  }
  else
  {
    USBD_GetString((uint8_t *)USBD_PRODUCT_FS_STRING_CDC, USBD_StrDesc_CDC, length);
  }
  return USBD_StrDesc_CDC;
}

/**
  * @brief  Returns the manufacturer string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_CDC_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING_CDC, USBD_StrDesc_CDC, length);
  return USBD_StrDesc_CDC;
}

/**
  * @brief  Returns the serial number string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_CDC_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  *length = USB_SIZ_STRING_SERIAL_CDC;

  /* Update the serial number string descriptor with the data from the unique ID*/
  Get_SerialNum();			//this crashes - reads IDs from wrong memory!

  return (uint8_t*)USBD_StringSerial_CDC;
}

/**
  * @brief  Returns the configuration string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_CDC_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  if(speed == USBD_SPEED_HIGH)
  {
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_HS_STRING_CDC, USBD_StrDesc_CDC, length);
  }
  else
  {
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_FS_STRING_CDC, USBD_StrDesc_CDC, length);
  }
  return USBD_StrDesc_CDC;
}

/**
  * @brief  Returns the interface string descriptor.
  * @param  speed: Current device speed
  * @param  length: Pointer to data length variable
  * @retval Pointer to descriptor buffer
  */
uint8_t *USBD_CDC_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
  if(speed == USBD_SPEED_HIGH)
  {
    USBD_GetString((uint8_t *)USBD_INTERFACE_HS_STRING_CDC, USBD_StrDesc_CDC, length);
  }
  else
  {
    USBD_GetString((uint8_t *)USBD_INTERFACE_FS_STRING_CDC, USBD_StrDesc_CDC, length);
  }
  return USBD_StrDesc_CDC;
}

/**
  * @brief  Create the serial number string descriptor
  * @param  None
  * @retval None
  */
static void Get_SerialNum(void)
{
  uint32_t deviceserial0, deviceserial1;
  uint32_t deviceserial2;

#if 1
  //this is a unique device from MCU chip; provide the address where to read from
  deviceserial0 = *(uint32_t*)DEVICE_ID1_CDC;
  deviceserial1 = *(uint32_t*)DEVICE_ID2_CDC;
  deviceserial2 = *(uint32_t*)DEVICE_ID3_CDC;
#else
  //create a fix one:
  deviceserial0 = 0x00000001;
  deviceserial1 = 0x00000001;
  deviceserial2 = 0x00000001;
#endif

  deviceserial0 += deviceserial2;

  if (deviceserial0 != 0)
  {
    IntToUnicode (deviceserial0, (uint8_t*)&USBD_StringSerial_CDC[2],  8);	//uni-code is 2bytes
    IntToUnicode (deviceserial1, (uint8_t*)&USBD_StringSerial_CDC[18], 4);	//take just the upper two bytes
  }
}

/**
  * @brief  Convert Hex 32Bits value into char
  * @param  value: value to convert
  * @param  pbuf: pointer to the buffer
  * @param  len: buffer length
  * @retval None
  */
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
  uint8_t idx = 0;

  for( idx = 0; idx < len; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10;
    }

    value = value << 4;

    pbuf[ 2* idx + 1] = 0;
  }
}
