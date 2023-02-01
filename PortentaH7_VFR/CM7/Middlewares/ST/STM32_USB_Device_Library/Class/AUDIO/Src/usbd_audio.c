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

#include "cmsis_os.h"

//#ifdef WITH_USB_AUDIO

/* Includes ------------------------------------------------------------------*/
//#include "Mem2MemDMA.h"
#include "usbd_conf.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "usbd_audio.h"

#ifdef WITH_PYACCEL
extern void fast_memcpy(void *dst, void *src, int bytes);
#endif
extern void fast_memset(void *dest, unsigned int val, int bytes);

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
#define AUDIO_SAMPLE_FREQ(frq)      (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq)       (uint8_t)(((frq * 2 * 2)/1000) & 0xFF), \
                                    (uint8_t)((((frq * 2 * 2)/1000) >> 8) & 0xFF)

////extern uint32_t EP1PktLen;

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

USBD_ClassTypeDef USBD_AUDIO_IN =
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

/* USB audio from device to host - input */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDescIn[USB_AUDIO_CONFIG_DESC_SIZE_IN] __ALIGN_END =
{
0x09, /* bLength */
USB_DESC_TYPE_CONFIGURATION,          	/* bDescriptorType */
LOBYTE(USB_AUDIO_CONFIG_DESC_SIZE_IN),	/* wTotalLength */
HIBYTE(USB_AUDIO_CONFIG_DESC_SIZE_IN),
0x02,									/* bNumInterfaces */
0x01,									/* bConfigurationValue */
0x00,									/* iConfiguration */
0xC0,									/* 0x80: bmAttributes BUS Powered, 0xC0: self-powered */
0x32,									/* bMaxPower = 100 mA*/  //0x0a

/* USB Speaker Standard AC Interface descriptor */
0x09,
0x04,
0x00,
0x00,
0x00,
0x01,
0x01,
0x00,
0x00,

/* Class specific AC interface descriptor */
0x09,
0x24,
0x01,
0x00,	/* bcdADC */
0x01,									/* Audio Device compliant to USB Audio 1.00 */
0x1E,	/* wTotalLength - 30 */
0x00,
0x01, 	/* bInCollection */				//bInCollection - number of streaming interfaces
0x01, 	/* baInterfaceNr(1) */

/* Input Terminal Descriptor */
0x0C, /* bLength */
0x24, /* bDescriptorType */
0x02, /* bDescriptorSubType */
0x01, /* bTerminalID */
0x01,	//0x02, /* wTerminalType */
0x02,	//0x06, /* wTerminalType */		//0x0201 = Microphone  //0x0401 = bidirectional head set, 0x0602 = Digital generic device
										//0x0603 = Line Interface, 0x0605 = SPDIF interface
0x00, /* bAssocTerminal */
0x02, /* bNrChannels */      			// <---- 0x02,  // two channels //0x01
0x03, /* wChannelConfig */   			// <---- 0x03,  // left front / right front
0x00, /* wChannelConfig */
0x00, /* iChannelName */
0x00, /* iTerminal */

/* Output Terminal Descriptor */
0x09, /* bLength */
0x24, /* bDescriptorType */
0x03, /* bDescriptorSubType */
0x02, /* bTerminalID */
0x01, /* wTerminalType */
0x01, /* wTerminalType */
0x00, /* bAssocTerminal */
0x01, /* bNrChannels */
0x00, /* wChannelConfig */

/* Standard AS Interface Descriptor (Alt. Set. 0) */
0x09, /* bLength */
0x04, /* bDescriptorType */
0x01, /* bInterfaceNum */
0x00, /* bAlternateSetting */
0x00, /* bNumEndpoints */
0x01, /* bInterfaceClass */
0x02, /* bInterfaceSubClass */
0x00, /* bInterfaceProtocol */
0x00, /* iInterface */

/* Standard AS Interface Descriptor (Alt. Set. 1) */
0x09, /* bLength */
0x04, /* bDescriptorType */
0x01, /* bInterfaceNum */
0x01, /* bAlternateSetting */
0x01, /* bNumEndpoints */
0x01, /* bInterfaceClass */
0x02, /* bInterfaceSubClass */
0x00, /* bInterfaceProtocol */
0x00, /* iInterface */

/* Class specific AS General Interface Descriptor */
0x07, /* bLength */
0x24, /* bDescriptorType */
0x01, /* bDescriptorSubType */
0x02, /* bTerminalLink */
0x00, /* bDelay */
0x01, /* wFormatTag */
0x00, /* wFormatTag */

/* Format Type Descriptor - the same as following */
//0x0B,
//0x24,
//0x02,
//0x01,		//0x01
//0x01,     /* bNrChannels */  // <---- 0x02, //Two channels //0x01
//0x02,
//0x10,
//0x01,
//0x44,     /*tSamFreq[0] */   // <---- 0x80,  // 48 kHz 	//0x40 //8 KHz  0x44  //44.1 KHz
//0xac,                        // <---- 0xBB,				//0x1f			0xAC
//0x00,

/* USB Speaker Audio Type III Format Interface Descriptor */
11,									  /* bLength */
AUDIO_INTERFACE_DESCRIPTOR_TYPE,      /* bDescriptorType */
AUDIO_STREAMING_FORMAT_TYPE,          /* bDescriptorSubtype */
AUDIO_FORMAT_TYPE_I,                  /* bFormatType */	//<---- AUDIO_FORMAT_TYPE_III or AUDIO_FORMAT_TYPE_I
0x02,	//0x0A,                       /* bNrChannels */	//<---- 10
0x02,                                 /* bSubFrameSize :  2 Bytes per frame (16bits) */   	//<---- 0x02
16,                                   /* bBitResolution (16-bits per sample) */				//<---- 16
0x01,                                 /* bSamFreqType only one frequency supported */
AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),   /* Audio sampling frequency coded on 3 bytes */
/* 11 byte*/

/* Endpoint Descriptor */
0x07,								  /* bLength */
0x05,								  /* bDescriptorType */
AUDIO_IN_EP,						  /* bEndpointAddress */
0x01,								  /* bmAttributes <---- 0x01, 0x05/0x0D invalid  // isoc, Synchronous //0x01 */
0xC0,	//0xC0,								  /* wMaxPacketSize <---- 0x03C0,  // 960 bytes (2 bytes per ch * 10 ch * 48 [KHz] samples / frame) */
0x00,	//0x03,								  /* wMaxPacketSize */
0x01,								  /* bInterval */
//0x00,								  /* bRefresh - UNKNOWN */
//0x00,								  /* bSynchAddress UNKNOWN */

/* Audio Data Endpoint Descriptor */
0x07,
0x25,
0x01,
0x00,
0x02,								  /* bLockDelayUnits <---- 0x02 = Decoded PCM samples, 0x00 */
0x00,								  /* wLockDelay */
0x00
};

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

uint8_t *USBD_AUDIO_CfgDesc = USBD_AUDIO_CfgDescIn;
int USBD_AUDIO_CfgDescSize = sizeof(USBD_AUDIO_CfgDescIn);

/**
  * @}
  */ 

/** @defgroup USBD_AUDIO_Private_Functions
  * @{
  */ 

static USBD_AUDIO_HandleTypeDef usbd_AUDIO_Handle;

////static unsigned long sUSBFrameCnt = 0;

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
  
  /* Open EP In */
  USBD_LL_OpenEP(pdev,
                 AUDIO_IN_EP,
                 USBD_EP_TYPE_ISOC,
                 AUDIO_IN_PACKET);

  /* Allocate Audio structure */
  ////pdev->pClassData = USBD_malloc(sizeof (USBD_AUDIO_HandleTypeDef));
  pdev->pClassData = &usbd_AUDIO_Handle;
  memset(&usbd_AUDIO_Handle, 0, sizeof (USBD_AUDIO_HandleTypeDef));
  
  if(pdev->pClassData == NULL)
  {
	  return USBD_FAIL;
  }
  else
  {
    haudio = (USBD_AUDIO_HandleTypeDef*)pdev->pClassData;
    haudio->alt_setting = 0;
    haudio->offset = AUDIO_OFFSET_UNKNOWN;
    haudio->wr_ptr = 0; 
    haudio->rd_ptr = 0;  
    haudio->rd_enable = 0;
    
    /* Initialize the Audio output Hardware layer */
    if (((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[pdev->classId])->Init(USBD_AUDIO_FREQ, 100, 0) != USBD_OK)			//XXXX
    {
    	return USBD_FAIL;
    }
    
    /* Prepare Out endpoint to receive 1st packet */ 
    USBD_LL_PrepareReceive(pdev,
                           AUDIO_OUT_EP,
                           haudio->buffer,                        
                           AUDIO_OUT_PACKET);      
  }
  return USBD_OK;
}

/**
  * @brief  USBD_AUDIO_DeInit
  *         DeInitialize the AUDIO layer
  * @param  pdev: device instance
  * @param  cfgidx: Configuration index
  * @retval status
  */
static uint8_t  USBD_AUDIO_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  (void)cfgidx;
  
  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, AUDIO_OUT_EP);

  /* Close EP IN */
  USBD_LL_CloseEP(pdev, AUDIO_IN_EP);

  /* DeInit  physical Interface components */
  if(pdev->pClassData != NULL)
  {
	  ((USBD_AUDIO_ItfTypeDef *)pdev->pUserData[pdev->classId])->DeInit(0);
	  //USBD_free(pdev->pClassData);
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
        pbuf = USBD_AUDIO_CfgDesc + 18;
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
static uint8_t *USBD_AUDIO_GetCfgDesc (uint16_t *length)
{
  *length = (uint16_t)USBD_AUDIO_CfgDescSize;
  return USBD_AUDIO_CfgDesc;
}

/* align for 32bit word access possible - align also with cache line size 32 */
static uint8_t USBInBuf[2u * AUDIO_TOTAL_BUF_SIZE] __ALIGNED(32) /* __attribute__((section(".dtcmram"))) */;
#define USB_BUF_ADDR	(&USBInBuf[0])
#define USB_BUF_SIZE	(sizeof(USBInBuf))

static unsigned int USBInIdxRd = 0;
////static unsigned int USBInIdxWr = AUDIO_TOTAL_BUF_SIZE;	//they should be always the opposite

/**
  * @brief  USBD_AUDIO_DataIn
  *         handle data IN Stage
  * @param  pdev: device instance
  * @param  epnum: endpoint index
  * @retval status
  */
static uint8_t __attribute__((section(".itcmram"))) USBD_AUDIO_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  (void)epnum;

  USBD_StatusTypeDef stat;

  ////USBD_LL_FlushEP(pdev, AUDIO_IN_EP);
  stat = USBD_LL_Transmit(pdev, AUDIO_IN_EP, (uint8_t*)(USB_BUF_ADDR + USBInIdxRd), AUDIO_IN_PACKET);	//length in bytes

  USBInIdxRd += AUDIO_IN_PACKET;
  if (USBInIdxRd >= USB_BUF_SIZE)
	  USBInIdxRd = 0;

#if TIMING_DEBUG
  if (USBInIdxRd)
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
  else
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
#endif

#if 0
  USBD_AUDIO_CpyBuf(USBInIdxRd);
#endif

  return (uint8_t)stat;
}

int16_t *GET_USBBuffer(void)
{
	return (int16_t *)(USB_BUF_ADDR + USBInIdxRd);
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
static uint8_t __attribute__((section(".itcmram")))  USBD_AUDIO_EP0_TxReady (USBD_HandleTypeDef *pdev)
{
  /* Only OUT control data are processed */
  USBD_LL_FlushEP(pdev, AUDIO_IN_EP);	//very important!!!

  USBInIdxRd = 0;
  ////USBInIdxWr = 0;

  ////USBD_AUDIO_ClearBuffer();

  ////USBD_AUDIO_CpyBuf();

  USBD_LL_Transmit(pdev, AUDIO_IN_EP, (uint8_t *)(USB_BUF_ADDR + USBInIdxRd), AUDIO_IN_PACKET);	//length in bytes
  USBInIdxRd += AUDIO_IN_PACKET;
  if (USBInIdxRd >= USB_BUF_SIZE)
	  USBInIdxRd = 0;

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

/**
  * @brief  USBD_AUDIO_SOF
  *         handle SOF event
  * @param  pdev: device instance
  * @retval status
  */
void  USBD_AUDIO_Sync (USBD_HandleTypeDef *pdev, AUDIO_OffsetTypeDef offset)
{
  (void)pdev;
  (void)offset;

#if 0
  int8_t shift = 0;
  USBD_AUDIO_HandleTypeDef   *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pClassData;
  
  haudio->offset =  offset; 
  
  if(haudio->rd_enable == 1)
  {
    haudio->rd_ptr += AUDIO_TOTAL_BUF_SIZE/2;
    
    if (haudio->rd_ptr == AUDIO_TOTAL_BUF_SIZE)
    {
      /* roll back */
      haudio->rd_ptr = 0;
    }
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
#endif
}

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
static uint8_t  USBD_AUDIO_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  (void)pdev;
  (void)epnum;

  return USBD_OK;
}

/**
  * @brief  AUDIO_Req_GetCurrent
  *         Handles the GET_CUR Audio control request.
  * @param  pdev: instance
  * @param  req: setup class request
  * @retval status
  */
static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
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
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
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
static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_AUDIO_DeviceQualifierDesc);
  return USBD_AUDIO_DeviceQualifierDesc;
}

/**
* @brief  USBD_AUDIO_RegisterInterface
* @param  fops: Audio interface callback
* @retval status
*/
uint8_t  USBD_AUDIO_RegisterInterface  (USBD_HandleTypeDef   *pdev, 
                                        USBD_AUDIO_ItfTypeDef *fops)
{
#if 0
  if(fops != NULL)
  {
    pdev->pUserData = fops;
  }
  return 0;
#else
  if (fops == NULL)
  {
    return (uint8_t)USBD_FAIL;
  }

  pdev->pUserData[pdev->classId] = fops;

  return (uint8_t)USBD_OK;
#endif
}

#if defined(AUDIO_DEMO) && defined(WITH_USB_AUDIO)
int16_t sineSamples[AUDIO_TOTAL_BUF_SIZE/sizeof(uint16_t)] = {
		0,	0,
		4277,	4277,
		8481,	8481,
		12539,	12539,
		16384,	16384,
		19947,	19947,
		23170,	23170,
		25996,	25996,
		28377,	28377,
		30273,	30273,
		31650,	31650,
		32487,	32487,
		32767,	32767,
		32487,	32487,
		31650,	31650,
		30273,	30273,
		28377,	28377,
		25996,	25996,
		23170,	23170,
		19947,	19947,
		16384,	16384,
		12539,	12539,
		8481,	8481,
		4277,	4277,
		0,	0,
		-4277,	-4277,
		-8481,	-8481,
		-12539,	-12539,
		-16384,	-16384,
		-19947,	-19947,
		-23170,	-23170,
		-25996,	-25996,
		-28377,	-28377,
		-30273,	-30273,
		-31650,	-31650,
		-32487,	-32487,
		-32767,	-32767,
		-32487,	-32487,
		-31650,	-31650,
		-30273,	-30273,
		-28377,	-28377,
		-25996,	-25996,
		-23170,	-23170,
		-19947,	-19947,
		-16384,	-16384,
		-12539,	-12539,
		-8481,	-8481,
		-4277,	-4277
};
#endif

#ifdef WITH_USB_AUDIO
void __attribute__((section(".itcmram"))) USBD_AUDIO_CpyBuf(unsigned int idx)
{
#if defined(AUDIO_DEMO)
#if 1
	//copy into unused buffer - works on all memories
	//fast_memcpy((void *)(USB_BUF_ADDR + USBInIdxWr), sineSamples, AUDIO_TOTAL_BUF_SIZE);
	//memcpy((void *)(USB_BUF_ADDR /*+ USBInIdxWr*/), sineSamples, (size_t)AUDIO_TOTAL_BUF_SIZE);
	memcpy((void *)(USB_BUF_ADDR + idx), sineSamples, (size_t)AUDIO_TOTAL_BUF_SIZE);
#else
	//works only on cache-coherent memories, DTCM!
	DMA_Mem2Mem_Start((uint32_t)sineSamples, (uint32_t)(USB_BUF_ADDR + USBInIdxWr), AUDIO_TOTAL_BUF_SIZE / 4);
#endif
#else
	extern int16_t *PCM_GetBuffer(void);
	memcpy((void *)(USB_BUF_ADDR + idx), (void *)PCM_GetBuffer(), (size_t)AUDIO_TOTAL_BUF_SIZE);
#if 0
	//if MMU configures cache-able with write through - not needed
	SCB_CleanDCache_by_Addr((uint32_t *)USB_BUF_ADDR + /*USBInIdxWr*/USBInIdxRd, AUDIO_TOTAL_BUF_SIZE);
#endif

#endif
#endif

	////USBInIdxWr += AUDIO_TOTAL_BUF_SIZE;
	////if (USBInIdxWr >= USB_BUF_SIZE)
	////	USBInIdxWr = 0;
}

void USBD_AUDIO_SetMode(int mode)
{
	if ( ! mode)
	{
		//PC USB Audio Out
#if 0
		extern uint8_t USBD_AUDIO_CfgDescOut[USB_AUDIO_CONFIG_DESC_SIZE_OUT];
		USBD_AUDIO_CfgDesc = USBD_AUDIO_CfgDescOut;
		USBD_AUDIO_CfgDescSize = sizeof(USBD_AUDIO_CfgDescOut);
#endif
	}
	if (mode == 1)
	{
		//PC USB Audio In
		USBD_AUDIO_CfgDesc = USBD_AUDIO_CfgDescIn;
		USBD_AUDIO_CfgDescSize = sizeof(USBD_AUDIO_CfgDescIn);
	}
	if (mode == 2)
	{
		//MSC device - handled separately
	}
	//mode = 3 is PC USB Audio Out (MCU will receive)
	//mode = 4 is VCP UART
}

#if 0
void USBD_AUDIO_ClearBuffer(void)
{
	/* disable INT - done on upper level */

	////USBInIdxRd = 0;	/* already done before called */
	////USBInIdxWr = 0;
	sUSBFrameCnt = 1;

	/* double buffer 1: */
	*((unsigned long *)(USB_BUF_ADDR)) = 0;							/* sUSBFrameCnt = 0 */
	*((unsigned long *)(USB_BUF_ADDR + 4)) = 0xFFFFFFFF;			/* invalid */
	memset(USB_BUF_ADDR + 8, 0, AUDIO_TOTAL_BUF_SIZE - 8);			/* clear all */

	/* double buffer 2: */
	*((unsigned long *)(USB_BUF_ADDR + AUDIO_TOTAL_BUF_SIZE)) = 1; 					/* sUSBFrameCnt = 1 */
	*((unsigned long *)(USB_BUF_ADDR + AUDIO_TOTAL_BUF_SIZE + 4)) = 0xFFFFFFFF;		/* invalid */
	memset(USB_BUF_ADDR + AUDIO_TOTAL_BUF_SIZE + 8, 0, AUDIO_TOTAL_BUF_SIZE - 8); 	/* clear all */

	SCB_CleanDCache_by_Addr((uint32_t *)USB_BUF_ADDR, USB_BUF_SIZE);
}
#endif

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
//#endif
