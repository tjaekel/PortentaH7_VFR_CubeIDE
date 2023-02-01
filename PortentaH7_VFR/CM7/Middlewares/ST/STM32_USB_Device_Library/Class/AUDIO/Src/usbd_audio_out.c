/*
 * usbd_audio_out.c
 *
 *  Created on: Jul 9, 2018
 *      Author: Torsten
 */

/**
  ******************************************************************************
  * @file    usbd_audio.c
  * @author  MCD Application Team
  * @version V2.4.2
  * @date    11-December-2015
  * @brief   This file provides the Audio core functions.
  *
  * @verbatim
  *
  *          ===================================================================
  *                                AUDIO Class  Description
  *          ===================================================================
 *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
  *           Audio Devices V1.0 Mar 18, 98".
  *           This driver implements the following aspects of the specification:
  *             - Device descriptor management
  *             - Configuration descriptor management
  *             - Standard AC Interface Descriptor management
  *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
  *             - 1 Audio Streaming Endpoint
  *             - 1 Audio Terminal Input (1 channel)
  *             - Audio Class-Specific AC Interfaces
  *             - Audio Class-Specific AS Interfaces
  *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
  *             - Audio Feature Unit (limited to Mute control)
  *             - Audio Synchronization type: Asynchronous
  *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
  *          The current audio class version supports the following audio features:
  *             - Pulse Coded Modulation (PCM) format
  *             - sampling rate: 48KHz.
  *             - Bit resolution: 16
  *             - Number of channels: 2
  *             - No volume control
  *             - Mute/Unmute capability
  *             - Asynchronous Endpoints
  *
  * @note     In HS mode and when the DMA is used, all variables and data structures
  *           dealing with the DMA during the transaction process should be 32-bit aligned.
  *
  *
  *  @endverbatim
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_audio.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_audio.h"


/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */


/** @defgroup USBD_AUDIO
  * @brief usbd core module
  * @{
  */

/** @defgroup USBD_AUDIO_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Defines
  * @{
  */

/**
  * @}
  */


/** @defgroup USBD_AUDIO_Private_Macros
  * @{
  */
#define AUDIO_SAMPLE_FREQ(frq)      	(uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)          	(uint8_t)(((frq * 2 * 2)/1000) & 0xFF), \
                                       	(uint8_t)((((frq * 2 * 2)/1000) >> 8) & 0xFF)

/**
  * @}
  */




/** @defgroup USBD_AUDIO_Private_FunctionPrototypes
  * @{
  */


static uint8_t  USBD_AUDIO_Init (USBD_HandleTypeDef *pdev,
                               uint8_t cfgidx);

static uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev,
                                 uint8_t cfgidx);

static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req);

static uint8_t  *USBD_AUDIO_GetCfgDesc (uint16_t *length);

static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length);

static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_EP0_RxReady (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_SOF (USBD_HandleTypeDef *pdev);

static uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

static uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum);

static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Variables
  * @{
  */

USBD_ClassTypeDef  USBD_AUDIO_OUT =
{
  USBD_AUDIO_Init,
  USBD_AUDIO_DeInit,
  USBD_AUDIO_Setup,
  USBD_AUDIO_EP0_TxReady,
  USBD_AUDIO_EP0_RxReady,
  USBD_AUDIO_DataIn,
  USBD_AUDIO_DataOut,
  USBD_AUDIO_SOF,
  USBD_AUDIO_IsoINIncomplete,
  USBD_AUDIO_IsoOutIncomplete,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetCfgDesc,
  USBD_AUDIO_GetDeviceQualifierDesc,
};

/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_AUDIO_CfgDescOut[USB_AUDIO_CONFIG_DESC_SIZE_OUT] __ALIGN_END =
{
  /* Configuration 1 */
  0x09,                                 /* bLength */
  USB_DESC_TYPE_CONFIGURATION,          /* bDescriptorType */
  LOBYTE(USB_AUDIO_CONFIG_DESC_SIZE_OUT),    /* wTotalLength  109 bytes*/
  HIBYTE(USB_AUDIO_CONFIG_DESC_SIZE_OUT),
  0x02,                                 /* bNumInterfaces */
  0x01,                                 /* bConfigurationValue */
  0x00,                                 /* iConfiguration */
  0xC0,                                 /* bmAttributes  BUS Powred*/
  0x32,                                 /* bMaxPower = 100 mA*/
  /* 09 byte*/

  /* USB Speaker Standard interface descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  USB_DESC_TYPE_INTERFACE,              /* bDescriptorType */
  0x00,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL,          /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Speaker Class-specific AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE,            /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_HEADER,                 /* bDescriptorSubtype */
  0x00,          /* 1.00 */             /* bcdADC */
  0x01,
  0x27,                                 /* wTotalLength = 39*/
  0x00,
  0x01,                                 /* bInCollection */
  0x01,                                 /* baInterfaceNr */
  /* 09 byte*/

  /* USB Speaker Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,       /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,         /* bDescriptorSubtype */
  0x01,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
  0x01,
  0x00,                                 /* bAssocTerminal */
  0x01,                                 /* bNrChannels */
  0x00,                                 /* wChannelConfig 0x0000  Mono */
  0x00,
  0x00,                                 /* iChannelNames */
  0x00,                                 /* iTerminal */
  /* 12 byte*/

  /* USB Speaker Audio Feature Unit Descriptor */
  0x09,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,           /* bDescriptorSubtype */
  AUDIO_OUT_STREAMING_CTRL,             /* bUnitID */
  0x01,                                 /* bSourceID */
  0x01,                                 /* bControlSize */
  AUDIO_CONTROL_MUTE,// |AUDIO_CONTROL_VOLUME, /* bmaControls(0) */
  0,                                    /* bmaControls(1) */
  0x00,                                 /* iTerminal */
  /* 09 byte*/

  /*USB Speaker Output Terminal Descriptor */
  0x09,      /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,        /* bDescriptorSubtype */
  0x03,                                 /* bTerminalID */
  0x01,                                 /* wTerminalType  0x0301*/
  0x03,
  0x00,                                 /* bAssocTerminal */
  0x02,                                 /* bSourceID */
  0x00,                                 /* iTerminal */
  /* 09 byte*/

  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwith */
  /* Interface 1, Alternate Setting 0                                             */
  AUDIO_INTERFACE_DESC_SIZE,  			/* bLength */
  USB_DESC_TYPE_INTERFACE,        		/* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x00,                                 /* bAlternateSetting */
  0x00,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Interface 1, Alternate Setting 1                                           */
  AUDIO_INTERFACE_DESC_SIZE,  /* bLength */
  USB_DESC_TYPE_INTERFACE,        /* bDescriptorType */
  0x01,                                 /* bInterfaceNumber */
  0x01,                                 /* bAlternateSetting */
  0x01,                                 /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,               /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,        /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,             /* bInterfaceProtocol */
  0x00,                                 /* iInterface */
  /* 09 byte*/

  /* USB Speaker Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,  /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,              /* bDescriptorSubtype */
  0x01,                                 /* bTerminalLink */
  0x01,                                 /* bDelay */
  0x01,                                 /* wFormatTag AUDIO_FORMAT_PCM  0x0001*/
  0x00,
  /* 07 byte*/

  /* USB Speaker Audio Type III Format Interface Descriptor */
  0x0B,                                 /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                	/* bFormatType */
  0x02,                                 /* bNrChannels */
  0x02,                                 /* bSubFrameSize :  2 Bytes per frame (16bits) */
  16,                                   /* bBitResolution (16-bits per sample) */
  0x01,                                 /* bSamFreqType only one frequency supported */
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),   /* Audio sampling frequency coded on 3 bytes */
  /* 11 byte*/

  /* Endpoint 1 - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,    /* bLength */
  USB_DESC_TYPE_ENDPOINT,               /* bDescriptorType */
  AUDIO_OUT_EP,                         /* bEndpointAddress 1 out endpoint*/
  USBD_EP_TYPE_ISOC,                    /* bmAttributes */
  AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),    /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  0x01,                                 /* bInterval */
  0x00,                                 /* bRefresh - UNKNOWN */
  0x00,                                 /* bSynchAddress - UNKNOWN */
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor*/
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE,   /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,               /* bDescriptor */
  0x00,                                 /* bmAttributes */
  0x00,                                 /* bLockDelayUnits */
  0x00,                                 /* wLockDelay */
  0x00,
  /* 07 byte*/
} ;

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END=
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

USBD_AUDIO_HandleTypeDef USBD_Audio_Handle /*__attribute((section("DTCM")))*/ ;
/* do not place it on DTCM, save the space for main audio */

/**
  * @}
  */

/** @defgroup USBD_AUDIO_Private_Functions
  * @{
  */

/**
  * @brief  USBD_AUDIO_Init
  *         Initialize the AUDIO interface
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  (void)cfgidx;
  USBD_AUDIO_HandleTypeDef   *haudio;

  /* Open EP OUT */
  USBD_LL_OpenEP(pdev,
                 AUDIO_OUT_EP,
                 USBD_EP_TYPE_ISOC,
                 AUDIO_OUT_PACKET);

  /* Allocate Audio structure */
  ////pdev->pClassData = USBD_malloc(sizeof (USBD_AUDIO_HandleTypeDef));
  pdev->pClassData = &USBD_Audio_Handle;
  /*
   * if DTCM is not initialized - we had to do it here
   */
  memset(&USBD_Audio_Handle, 0, sizeof(USBD_AUDIO_HandleTypeDef));

  if(pdev->pClassData == NULL)
  {
    return USBD_FAIL;
  }
  else
  {
    haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
    haudio->alt_setting = 0;
    haudio->wr_ptr = haudio->rd_ptr = 0;

#if 0
    /* Initialize the Audio output Hardware layer */
    if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->Init(USBD_AUDIO_FREQ, AUDIO_DEFAULT_VOLUME, 0) != USBD_OK)
    {
      return USBD_FAIL;
    }
#endif

    /* Prepare Out endpoint to receive 1st packet */
    USBD_LL_PrepareReceive(pdev,
                           AUDIO_OUT_EP,
                           haudio->buffer,
                           AUDIO_OUT_PACKET);
  }
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Init
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  (void)cfgidx;

  /* Open EP OUT */
  USBD_LL_CloseEP(pdev,
              AUDIO_OUT_EP);

  /* DeInit  physical Interface components */
  if(pdev->pClassData != NULL)
  {
   ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[pdev->classId])->DeInit(0);
    ////USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_Setup
  *         Handle the AUDIO specific requests
  * @param  pdev: instance
  * @param  req: usb requests
  * @retval status
  */
static uint8_t  USBD_AUDIO_Setup (USBD_HandleTypeDef *pdev,
                                USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint8_t ret = USBD_OK;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
  case USB_REQ_TYPE_CLASS :
    switch (req->bRequest)
    {
    case AUDIO_REQ_GET_CUR:
      AUDIO_REQ_GetCurrent(pdev, req);
      break;

    case AUDIO_REQ_SET_CUR:
      AUDIO_REQ_SetCurrent(pdev, req);
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
    }
    break;

  case USB_REQ_TYPE_STANDARD:
    switch (req->bRequest)
    {
    case USB_REQ_GET_DESCRIPTOR:
      if( (req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
      {
        pbuf = USBD_AUDIO_CfgDescOut + 18;
        len = MIN(USB_AUDIO_DESC_SIZ , req->wLength);


        USBD_CtlSendData (pdev,
                          pbuf,
                          len);
      }
      break;

    case USB_REQ_GET_INTERFACE :
      USBD_CtlSendData (pdev,
                        (uint8_t *)&(haudio->alt_setting),
                        1);
      break;

    case USB_REQ_SET_INTERFACE :
      if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES)
      {
        haudio->alt_setting = (uint8_t)(req->wValue);
      }
      else
      {
        /* Call the error management function (command will be nacked */
        USBD_CtlError (pdev, req);
      }
      break;

    default:
      USBD_CtlError (pdev, req);
      ret = USBD_FAIL;
    }
  }
  return ret;
}


/**
  * @brief  USBD_AUDIO_GetCfgDesc
  *         return configuration descriptor
  * @param  speed : current device speed
  * @param  length : pointer data length
  * @retval pointer to descriptor buffer
  */
static uint8_t  * __attribute__((section(".itcmram"))) USBD_AUDIO_GetCfgDesc (uint16_t *length)
{
  *length = sizeof (USBD_AUDIO_CfgDescOut);
  return USBD_AUDIO_CfgDescOut;
}

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  (void)pdev;
  (void)epnum;

  /* Only OUT data are processed */
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_EP0_RxReady
  *         handle EP0 Rx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t __attribute__((section(".itcmram"))) USBD_AUDIO_EP0_RxReady (USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  if (haudio->control.cmd == AUDIO_REQ_SET_CUR)
  {/* In this driver, to simplify code, only SET_CUR request is managed */

    if (haudio->control.unit == AUDIO_OUT_STREAMING_CTRL)
    {
     ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[pdev->classId])->MuteCtl(haudio->control.data[0]);
      haudio->control.cmd = 0;
      haudio->control.len = 0;
    }
  }

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_EP0_TxReady
  *         handle EP0 TRx Ready event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev)
{
  (void)pdev;

  /* Only OUT control data are processed */
  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
static uint8_t  USBD_AUDIO_SOF (USBD_HandleTypeDef *pdev)
{
  (void)pdev;

  return USBD_OK;
}

#ifdef NOT_USED
/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
void  USBD_AUDIO_Sync (USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset)
{
  int8_t shift = 0;
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  haudio->offset =  offset;

  haudio->rd_ptr += AUDIO_TOTAL_BUF_SIZE/2;

  if (haudio->rd_ptr == AUDIO_TOTAL_BUF_SIZE)
  {
      /* roll back */
      haudio->rd_ptr = 0;
  }

  if(haudio->rd_ptr > haudio->wr_ptr)
  {
    if((haudio->rd_ptr - haudio->wr_ptr) < AUDIO_OUT_PACKET)
    {
      shift = -4;
    }
    else if((haudio->rd_ptr - haudio->wr_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
    {
      shift = 4;
    }

  }
  else
  {
    if((haudio->wr_ptr - haudio->rd_ptr) < AUDIO_OUT_PACKET)
    {
      shift = 4;
    }
    else if((haudio->wr_ptr - haudio->rd_ptr) > (AUDIO_TOTAL_BUF_SIZE - AUDIO_OUT_PACKET))
    {
      shift = -4;
    }
  }

  if(haudio->offset == AUDIO_OFFSET_FULL)
  {
    ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData)->AudioCmd(&haudio->buffer[0],
                                                         AUDIO_TOTAL_BUF_SIZE/2 - shift,
                                                         AUDIO_CMD_PLAY);
      haudio->offset = AUDIO_OFFSET_NONE;
  }
}
#endif

/**
  * @brief  USBD_AUDIO_IsoINIncomplete
  *         handle data ISO IN Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_IsoINIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  (void)pdev;
  (void)epnum;

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_IsoOutIncomplete
  *         handle data ISO OUT Incomplete event
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t  USBD_AUDIO_IsoOutIncomplete (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  (void)pdev;
  (void)epnum;

  return USBD_OK;
}
/**
  * @brief  USBD_AUDIO_DataOut
  *         handle data OUT Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t __attribute__((section(".itcmram"))) USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  uint16_t nextWr_ptr = haudio->wr_ptr;

  if (epnum == AUDIO_OUT_EP)
  {
    /* Increment the Buffer pointer or roll it over when all buffers are full */
	/* check if we would overrun our rd_ptr */
    nextWr_ptr += AUDIO_OUT_PACKET;
	if (nextWr_ptr == AUDIO_TOTAL_BUF_SIZE)
	    nextWr_ptr = 0;

	haudio->wr_ptr += AUDIO_OUT_PACKET;

	if (haudio->rd_ptr > haudio->wr_ptr)
		if (nextWr_ptr >= haudio->rd_ptr)
			/* avoid overrun of rd_ptr - skip next USB frame */
			haudio->wr_ptr -= AUDIO_OUT_PACKET;

    if (haudio->wr_ptr == AUDIO_TOTAL_BUF_SIZE)
    	/* All buffers are full: roll back */
    	haudio->wr_ptr = 0;

    /* Prepare Out endpoint to receive next audio packet */
    USBD_LL_PrepareReceive(pdev,
                           AUDIO_OUT_EP,
                           &haudio->buffer[haudio->wr_ptr],
                           AUDIO_OUT_PACKET);
  }

  return USBD_OK;
}

/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void __attribute__((section(".itcmram"))) AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  memset(haudio->control.data, 0, 64);
  /* Send the current mute state */
  USBD_CtlSendData (pdev,
                    haudio->control.data,
                    req->wLength);
}

/**
  * @brief  AUDIO_Req_SetCurrent
  *         Handles the SET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void __attribute__((section(".itcmram"))) AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;

  if (req->wLength)
  {
    /* Prepare the reception of the buffer over EP0 */
    USBD_CtlPrepareRx (pdev,
                       haudio->control.data,
                       req->wLength);

    haudio->control.cmd = AUDIO_REQ_SET_CUR;     /* Set the request value */
    haudio->control.len = (uint8_t)req->wLength;          /* Set the request data length */
    haudio->control.unit = HIBYTE(req->wIndex);  /* Set the request target unit */
  }
}


/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
static uint8_t  * __attribute__((section(".itcmram"))) USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_AUDIO_DeviceQualifierDesc);
  return USBD_AUDIO_DeviceQualifierDesc;
}

#if ALREADY_DEFINED
/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t  USBD_AUDIO_RegisterInterface  (USBD_HandleTypeDef   *pdev,
                                        USBD_AUDIO_ItfTypeDef *fops)
{
  if(fops != NULL)
  {
    pdev->pUserData= fops;
  }
  return 0;
}
#endif

static void * __attribute__((section(".itcmram"))) USBD_AUDIO_CopySlot(void *dst, void *src, unsigned int sizeBytes)
{
	uint16_t *pDst, *pSrc;
	pDst = (uint16_t *)dst;
	pSrc = (uint16_t *)src;

	while (sizeBytes > 0)
	{
		*pDst++ = *pSrc++; pDst++;
		*pDst++ = *pSrc++; pDst++;
		sizeBytes -= 8;
	}
	return pDst;
}

void __attribute__((section(".itcmram"))) USBD_AUDIO_CopyBuffer(uint16_t *dest, unsigned int sizeBytes)
{
	/*
	 * ATTENTION: we have 4 time slots, sizeBytes is for 4 time slots!
	 */

	/*
	 * ATTENTION: USB cannot be error free! I see USB is a bit faster as SAI Out - we have one USB packet too much,
	 * or we can overrun the rd_ptr and wr_ptr here, audible clicks and distortions possible!
	 * There is not any means to handle this issue: we cannot trim PLL dynamically or change the SAI I2S format on the fly.
	 * So, we have to live with small audible artefacts, unfortunately. The only correct solution is to have a PLL with
	 * fractional divider settings. An adaptive clock recovery or Clock Recovery System (CRS) is necessary in order to deal
	 * with two independent clock domains.
	 */

	register uint16_t sizeBytesUSB = (uint16_t)sizeBytes / 2;
	uint16_t wr_ptr;
	static uint16_t prevWr_ptr = 0;

	__disable_irq();
	wr_ptr = USBD_Audio_Handle.wr_ptr;

	/* check if USB has stopped, no reception */
	if (wr_ptr == prevWr_ptr)
	{
		USBD_Audio_Handle.rd_ptr = USBD_Audio_Handle.wr_ptr = 0;
	}
	prevWr_ptr = USBD_Audio_Handle.wr_ptr;
	__enable_irq();

	/* check if we have enough data */
	if (USBD_Audio_Handle.rd_ptr <= wr_ptr)
	{
		//wr_ptr has not wrapped around */
		if ((wr_ptr - USBD_Audio_Handle.rd_ptr) < sizeBytesUSB)
		{
			/* this should happen if USB does not run, stopped USB on host = nothing received */
			memset(dest, 0, (size_t)((uint16_t)sizeBytes - (wr_ptr - USBD_Audio_Handle.rd_ptr)*2));		/* in bytes */
			dest += sizeBytes - (unsigned int)((wr_ptr - USBD_Audio_Handle.rd_ptr));				/* dest is uint16_t, half the size */
			USBD_AUDIO_CopySlot(dest, &USBD_Audio_Handle.buffer[USBD_Audio_Handle.rd_ptr], (unsigned int)((wr_ptr - USBD_Audio_Handle.rd_ptr))*2u);
			USBD_Audio_Handle.rd_ptr += (wr_ptr - USBD_Audio_Handle.rd_ptr);
		}
		else
		{
			USBD_AUDIO_CopySlot(dest, &USBD_Audio_Handle.buffer[USBD_Audio_Handle.rd_ptr], sizeBytes);
			USBD_Audio_Handle.rd_ptr += (uint16_t)sizeBytesUSB;
		}
	}
	else
	{
#if 1
		/* wr_ptr should have wrapped around */
		if ((unsigned int)((sizeof(USBD_Audio_Handle.buffer) - USBD_Audio_Handle.rd_ptr + wr_ptr)) < sizeBytesUSB)
		{
			/* this happens very seldom or never */
			unsigned int s1, s2;
			s1 = (sizeof(USBD_Audio_Handle.buffer) - USBD_Audio_Handle.rd_ptr)*2;	//first part
			s2 = (unsigned int)wr_ptr*2;															//second part
			USBD_AUDIO_CopySlot(dest, &USBD_Audio_Handle.buffer[USBD_Audio_Handle.rd_ptr], s1);
			dest = USBD_AUDIO_CopySlot(dest + s1/sizeof(uint16_t), &USBD_Audio_Handle.buffer[0], s2);
			memset(dest, 0, sizeBytes - (s1 + s2));
			USBD_Audio_Handle.rd_ptr = (uint16_t)(s2/2);
		}
		else
#endif
		{
			unsigned int s1, s2;
			s1 = (sizeof(USBD_Audio_Handle.buffer) - USBD_Audio_Handle.rd_ptr)*2;	//first part
			s2 = sizeBytes - s1;													//second part
			USBD_AUDIO_CopySlot(dest, &USBD_Audio_Handle.buffer[USBD_Audio_Handle.rd_ptr], s1);
			USBD_AUDIO_CopySlot(dest + s1/sizeof(uint16_t), &USBD_Audio_Handle.buffer[0], s2);
			USBD_Audio_Handle.rd_ptr = (uint16_t)(s2/2);
		}
	}

	if (USBD_Audio_Handle.rd_ptr >= sizeof(USBD_Audio_Handle.buffer))
		USBD_Audio_Handle.rd_ptr = 0;
}

/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

