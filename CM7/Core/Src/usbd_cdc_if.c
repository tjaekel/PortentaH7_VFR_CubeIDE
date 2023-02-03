/**
 * usnd_cdc_if.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"
#include "cmsis_os.h"

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_CDC 
  * @brief usbd core module
  * @{
  */ 

/** @defgroup USBD_CDC_Private_TypesDefinitions
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Defines
  * @{
  */ 
/* Define size for the receive and transmit buffer over CDC */
/* It's up to user to redefine and/or remove those define */
#define APP_RX_DATA_SIZE  4096		//64 actually, it was 4, might be enough: for a complete Escape sequence
#define APP_TX_DATA_SIZE  4096		//64 not used: we can use to control max. USB VCP chunk size as 64
/**
  * @}
  */ 

/** @defgroup USBD_CDC_Private_Macros
  * @{
  */ 

/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_Private_Variables
  * @{
  */

/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/* Received Data over USB are stored in this buffer       */
static uint8_t  UserRxBufferHS[APP_RX_DATA_SIZE];
volatile static uint16_t UserRxBufferHSSize = 0;
volatile static uint16_t UserRxBufferHSIdx = 0;

/* Send Data over USB CDC are stored in this buffer       */
////uint8_t UserTxBufferHS[APP_TX_DATA_SIZE];

LINE_CODING linecoding = {
	115200, /* baud rate */
	0x00,   /* stop bits-1 */
	0x00,   /* parity - none */
	0x08,   /* number of bits 8 */
	1		/* Changed flag */
};

/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_IF_Exported_Variables
  * @{
  */ 
  extern USBD_HandleTypeDef USBD_Device;

/**
  * @}
  */ 
  
/** @defgroup USBD_CDC_Private_FunctionPrototypes
  * @{
  */

static int8_t CDC_Init_HS     (void);
static int8_t CDC_DeInit_HS   (void);
static int8_t CDC_Control_HS  (uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_HS  (uint8_t* pbuf, uint32_t *Len);

static int VCP_Ready = 0;

/**
  * @}
  */ 

USBD_CDC_ItfTypeDef USBD_CDC_fops =
{
  CDC_Init_HS,
  CDC_DeInit_HS,
  CDC_Control_HS,  
  CDC_Receive_HS
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  CDC_Init_HS
  *         Initializes the CDC media low layer over the USB HS IP
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_HS(void)
{
  /* Set Application Buffers */
  ////USBD_CDC_SetTxBuffer(&USBD_Device, UserTxBufferHS, 0);
  USBD_CDC_SetRxBuffer(&USBD_Device, UserRxBufferHS);

  VCP_Ready = 1;
  return (USBD_OK);
}

/**
  * @brief  CDC_DeInit_HS
  *         DeInitializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_HS(void)
{
  VCP_Ready = 0;
  return (USBD_OK);
}

/**
  * @brief  CDC_Control_HS
  *         Manage the CDC class requests
  * @param  cmd: Command code            
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_HS  (uint8_t cmd, uint8_t* pbuf, uint16_t length)
{ 
  switch (cmd)
  {
  case CDC_SEND_ENCAPSULATED_COMMAND:
 
    break;

  case CDC_GET_ENCAPSULATED_RESPONSE:
 
    break;

  case CDC_SET_COMM_FEATURE:
 
    break;

  case CDC_GET_COMM_FEATURE:

    break;

  case CDC_CLEAR_COMM_FEATURE:

    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */ 
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
  case CDC_SET_LINE_CODING: 
	linecoding.bitrate = (uint32_t)(pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) | (pbuf[3] << 24));
    linecoding.format = pbuf[4];
    linecoding.paritytype = pbuf[5];
    linecoding.datatype = pbuf[6];
    linecoding.changed = 1;  
	
    break;

  case CDC_GET_LINE_CODING:     
	linecoding.bitrate = 1843200;
    pbuf[0] = (uint8_t)(linecoding.bitrate);
    pbuf[1] = (uint8_t)(linecoding.bitrate >> 8);
    pbuf[2] = (uint8_t)(linecoding.bitrate >> 16);
    pbuf[3] = (uint8_t)(linecoding.bitrate >> 24);

    /* TODO: we can have more configuration here */
    pbuf[4] = linecoding.format;
    pbuf[5] = linecoding.paritytype;
    pbuf[6] = linecoding.datatype;

    break;

  case CDC_SET_CONTROL_LINE_STATE:

    break;

  case CDC_SEND_BREAK:
 
    break;    
    
  default:
    break;
  }

  return (USBD_OK);
}

/**
  * @brief  CDC_Transmit_HS
  *         Data send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be send
  * @param  Len: Number of data to be send (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_HS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)USBD_Device.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }
  USBD_CDC_SetTxBuffer(&USBD_Device, Buf, Len);
  result = USBD_CDC_TransmitPacket(&USBD_Device);

  return result;
}

/**
  * @brief  CDC_Receive_HS
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  *           
  *         @note
  *         This function will block any OUT packet reception on USB endpoint 
  *         until exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (i.e. using DMA controller) it will result
  *         in receiving more data while previous ones are still not sent.
  *                 
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_HS(uint8_t* Buf, uint32_t *Len)
{
  USBD_CDC_SetRxBuffer(&USBD_Device, &Buf[0]);
  UserRxBufferHSSize = *Len;

  USBD_CDC_ReceivePacket(&USBD_Device);
  return (USBD_OK);
}

uint16_t CDC_GetRx(uint8_t *Buf, uint16_t Len)
{
	/* we can have a complete ESC sequence in buffer */
	uint16_t len = 0;

	if ( ! VCP_Ready)
		return 0;

	len = UserRxBufferHSSize - UserRxBufferHSIdx;
	if (len)
	{
		if (len > Len)
			len = Len;
		memcpy(Buf, &UserRxBufferHS[UserRxBufferHSIdx], len);
		UserRxBufferHSIdx += len;
		if (UserRxBufferHSIdx == UserRxBufferHSSize)
		{
			UserRxBufferHSIdx = UserRxBufferHSSize = 0;
		}
		return len;
	}
	else
		return 0;
}

uint16_t CDC_PutTx(uint8_t *Buf, uint16_t Len)
{
	if ( ! VCP_Ready)
		return 0;

	while (CDC_Transmit_HS(Buf, Len) != USBD_OK)
	{
		osThreadYield();
	}

	return Len;
}

int CDC_isReady(void)
{
	return VCP_Ready;
}


//------------------------------- CDC2 ----------------------------------------------

extern USBD_HandleTypeDef USBD_Device2;

static int8_t CDC_Init_FS     (void);
static int8_t CDC_DeInit_FS   (void);
static int8_t CDC_Receive_FS  (uint8_t* pbuf, uint32_t *Len);

USBD_CDC_ItfTypeDef USBD_CDC_fops2 =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_HS,
  CDC_Receive_FS
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  CDC_Init_HS
  *         Initializes the CDC media low layer over the USB HS IP
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  /* Set Application Buffers */
  ////USBD_CDC_SetTxBuffer(&USBD_Device, UserTxBufferHS, 0);
  USBD_CDC_SetRxBuffer(&USBD_Device2, UserRxBufferHS);

  return (USBD_OK);
}

/**
  * @brief  CDC_DeInit_HS
  *         DeInitializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  return (USBD_OK);
}

/**
  * @brief  CDC_Transmit_HS
  *         Data send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be send
  * @param  Len: Number of data to be send (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)USBD_Device2.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }
  USBD_CDC_SetTxBuffer(&USBD_Device2, Buf, Len);
  result = USBD_CDC_TransmitPacket(&USBD_Device2);

  return result;
}

/**
  * @brief  CDC_Receive_HS
  *         Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will block any OUT packet reception on USB endpoint
  *         until exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (i.e. using DMA controller) it will result
  *         in receiving more data while previous ones are still not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  USBD_CDC_SetRxBuffer(&USBD_Device2, &Buf[0]);
  UserRxBufferHSSize = *Len;

  USBD_CDC_ReceivePacket(&USBD_Device2);
  return (USBD_OK);
}

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

