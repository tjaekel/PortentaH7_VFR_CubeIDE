/**
 * main.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#include "main.h"
#include "cmsis_os.h"

#include "usbd_core.h"
#include "usbd_cdc_if.h"
#include "usbd_desc_cdc.h"

#include "lwip/tcpip.h"
#include "ethernetif.h"
#include "app_ethernet.h"
#include "httpserver_netconn.h"

#include "I2C_PMIC.h"
#include "SPI.h"
#include "I2C3_user.h"
#include "GPIO_user.h"
#include "SDRAM.h"
#include "QSPI.h"
#include "SDCard.h"
#include "ADC3_temp.h"
#include "TFTP.h"

#include "PDM_MIC.h"

#include "cmd_dec.h"

#include "power_mngt.h"

/** TODO:
 * fix index for QSPI man pages
 * update man pages and flash QSPI chip
 * add PDM sine pattern and play instead of MICs (as reference)
 */

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

USBD_HandleTypeDef USBD_Device;
extern PCD_HandleTypeDef hpcd;

extern int SetSysClock_PLL_HSE(uint8_t bypass, int lowspeed);

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128,
  .priority = (osPriority_t) osPriorityIdle //osPriorityBelowNormal	//XXXX !!
};
osThreadId_t UARTTaskHandle;
const osThreadAttr_t UARTTask_attributes = {
  .name = "UARTTask",
  .stack_size = 256 * 128,						//XXXX !!
  .priority = (osPriority_t) osPriorityBelowNormal
};
osThreadId_t StartThreadHandle;
const osThreadAttr_t StartThread_attributes = {
  .name = "StartThread",
  .stack_size = 256 * 4,						//XXXX !!
  .priority = (osPriority_t) osPriorityNormal,
};

osThreadId_t EthernetHandle;
const osThreadAttr_t EthernetThread_attributes = {
  .name = "EthernetThread",
  .stack_size = 256 * 4,						//XXXX !!
  .priority = (osPriority_t) osPriorityAboveNormal,
};
osThreadId_t DHCPHandle;
const osThreadAttr_t DHCPThread_attributes = {
  .name = "DHCPThread",
  .stack_size = 256 * 4,						//XXXX !!
  .priority = (osPriority_t) osPriorityNormal
};

osThreadId_t GPIOINTHandle;
const osThreadAttr_t GPIOINT_attributes = {
  .name = "GPIOINTThread",
  .stack_size = 256 * 16,						//XXXX !! stack for Pico-C
  .priority = (osPriority_t) osPriorityNormal
};

osThreadId_t TOFHandle;
const osThreadAttr_t TOFThread_attributes = {
  .name = "TOFThread",
  .stack_size = 256 * 4,						//XXXX !!
  .priority = (osPriority_t) osPriorityNormal
};

osSemaphoreId xSemaphoreGPIOINT   = NULL;
osSemaphoreId xSemaphoreTOF		  = NULL;

static void MX_GPIO_Init(void);
static void MPU_Config(void);
#if 0
static void MX_CRC_Init(void);
CRC_HandleTypeDef hcrc;
#endif

void StartDefaultTask(void *argument);
void StartUARTTask(void *argument);
void StartThread(void *argument);
void GPIOINTTask(void *argument);
extern void StartTOFTask(void *argument);		//TOF sensor thread

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	//PMIC configuration - as earliest as possible
	if (I2C_PMIC_Setup())
		SYS_SetError(SYS_ERR_SYSCFG);
	if (I2C_PMIC_Initialize())
		SYS_SetError(SYS_ERR_SYSCFG);

	/* MPU for ETH buffers - not cache-able */
	MPU_Config();

#ifdef ENABLE_DCACHE
	SCB_EnableDCache();
#endif
	SCB_EnableICache();

#if 0
  int32_t timeout;
#endif
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
#if 0
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
	  Error_Handler();
  }
#endif

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

#ifdef ARDUINO_BOOTLOADER
  //just if using bootloader and start from 0x08040000
  ////unsigned long vectTable;

  ////vectTable = (unsigned long)SCB->VTOR;
  SCB->VTOR = (unsigned long)0x08040000;
  ////SCB->VTOR = (unsigned long)0x20000000;	//it is not copied yet! - the old bootloader table is there
  ////vectTable = (unsigned long)SCB->VTOR;
#endif

  if (SetSysClock_PLL_HSE(1, 0) == 0)
	  SYS_SetError(SYS_ERR_SYS_INIT);
  SystemCoreClockUpdate();

#if 0
  //seems to be enabled already, no effect, we use WFI in IdleThread - works the same way
  __HAL_RCC_DTCM1_CLK_SLEEP_ENABLE();
  __HAL_RCC_DTCM2_CLK_SLEEP_ENABLE();
  __HAL_RCC_ITCM_CLK_SLEEP_ENABLE();

  __HAL_RCC_D1SRAM1_CLK_SLEEP_ENABLE();
  __HAL_RCC_AXISRAM_CLK_SLEEP_ENABLE();
  __HAL_RCC_D1SRAM1_CLK_SLEEP_ENABLE();
  __HAL_RCC_D1SRAM1_CLK_SLEEP_ENABLE();

  __HAL_RCC_D3SRAM1_CLK_SLEEP_ENABLE();
  __HAL_RCC_FLASH_CLK_SLEEP_ENABLE();
#endif

  /* Initialize all configured peripherals - esp. LEDs */
  MX_GPIO_Init();
  GPIO_INTConfig(0, 1);
  GPIO_resConfig();

  /* read the syscfg */
  SYSCFG_Init();

  /* configure system with config - except all drivers with RTOS features, e.g. SPI! */
  GPIO_Set(GSysCfg.GPIOval, GSysCfg.GPIOmask);
  GPIO_Config(GSysCfg.GPIOdir, GSysCfg.GPIOod);

  ITM_enable();		//does it conflict with SDMMC2 PB3, SDC2_D3 and SWO?

#if 0
  /* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
     HSEM notification */
  /* HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  /*Take HSEM */
  HAL_HSEM_FastTake(HSEM_ID_0);
  /*Release HSEM in order to notify the CPU2(CM4)*/
  HAL_HSEM_Release(HSEM_ID_0,0);
  /* wait until CPU2 wakes up from stop mode */
  timeout = 0xFFFF;
  while((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0));
  if ( timeout < 0 )
  {
	  Error_Handler();
  }
#endif

  MEM_PoolInit();

  SDRAM_Init();  /* not immediately usable, takes a bit of time to have PLL and SRAM stable */

  QSPI_Init();
  QSPI_demo(0, 0);
  QSPI_demo(4, 0);

#if 1
  /* initialize USB VCP for UART */
  USBD_Init(&USBD_Device, &CDC_Desc, 0);
  USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
  USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
  USBD_Start(&USBD_Device);
#endif

  ADC3_Init();
#if 0
  MX_CRC_Init();
#endif

  /* Initialize RTOS scheduler */
   osKernelInitialize();

   xSemaphoreGPIOINT   = osSemaphoreNew(1, 0, NULL);
   xSemaphoreTOF	   = osSemaphoreNew(1, 0, NULL);

  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  /* add threads, ... */
  UARTTaskHandle = osThreadNew(StartUARTTask, NULL, &UARTTask_attributes);
  GPIOINTHandle  = osThreadNew(GPIOINTTask, NULL, &GPIOINT_attributes);
  TOFHandle  	 = osThreadNew(StartTOFTask, NULL, &TOFThread_attributes);

  if (HTTPD_OutInit())
	  /* Init thread for network - terminates itself */
	  StartThreadHandle = osThreadNew(StartThread, NULL, &StartThread_attributes);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  while (1)
  {
	  /* should not be executed! */
	  int i;
	  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_SET);
	  for (i = 0; i < 10000000; i++)
		  __NOP();
	  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_RESET);
	  for (i = 0; i < 10000000; i++)
		  __NOP();
  }
}

static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;

  /* Disable the MPU */
  HAL_MPU_Disable();

  /* entire D2 cachable */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;					//actually: just 288KB!
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;		//MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE; 		//MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE; 	//MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as cache-able write through
     for LwIP RAM heap which contains the Tx buffers
     ETH Tx buffer: 32KB
     ETH Rx buffer: 32KB
   */
  /* ETH Tx and Rx buffers - can be cache-able - 64KB */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30038000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE; 		//MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE; 	//MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as Device not-cache-able
     for ETH DMA descriptors: 2x 512 bytes = 1K */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x30037C00;
  MPU_InitStruct.Size = MPU_REGION_SIZE_1KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;		//MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE; 		//MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE; 	//MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /* Configure the MPU attributes as Cache-able write through
     for our main memory - for DMA coherancy ! */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x24000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;		//MPU_ACCESS_BUFFERABLE;		//it makes it a bit faster
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE; 		//MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER3;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE; 	//MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

#if 1
  /* entire SDRAM cachable */
  /**
   * it does not work: when done - the memory written is not ssen, so we leave it at it is,
   * should be already cacheable and WBWA as default
   */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_8MB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;		//MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE; 		//MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;					//is there a TEX level 4?
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE; 	//MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

#if 0
  /* Configure the MPU attributes as Cacheable write through
     for our D3 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x38000000;						//start D3 = 0x38000000
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;					//end D3 = 0x38010000
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE; 		//MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER4;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE; //MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
#endif

  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
/* we use the external function uint8_t SetSysClock_PLL_HSE(uint8_t bypass, int lowspeed) */

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	/* enable GPIO clocks - just to make sure all ready */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();
	__HAL_RCC_GPIOJ_CLK_ENABLE();
	__HAL_RCC_GPIOK_CLK_ENABLE();

#if 1
	/* hold unused components in reset or power down mode (WiFi, BT, Crypto */
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_12, GPIO_PIN_RESET);		//CRYPTO_EN, ENA
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_1, GPIO_PIN_RESET);		//WL_ON
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_12, GPIO_PIN_RESET);		//BT_ON
	//PI8 is an input as BT_SEL, from breakout board

	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; //GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_12;
	HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);
#endif

	/* LED config */
  	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_SET);		//red
  	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_SET);		//green
  	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_7, GPIO_PIN_SET);		//blue

  	GPIO_InitStruct.Pull = GPIO_NOPULL;
  	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; //GPIO_SPEED_FREQ_LOW;
  	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
  	HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);

  	/* do not enable the MIPI video USB bridge */
  	HAL_GPIO_WritePin(GPIOK, GPIO_PIN_2, GPIO_PIN_RESET);
  	GPIO_InitStruct.Pull = GPIO_NOPULL;
  	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; //GPIO_SPEED_FREQ_LOW;
  	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  	GPIO_InitStruct.Pin = GPIO_PIN_2;
  	HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);

  	/* disable VBUS on main board - we use as device, not as host - high is disable,
  	 * default: VBUS is enabled!
  	 */
  	HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_SET);		//VBUS disabled

  	GPIO_InitStruct.Pull = GPIO_NOPULL;
  	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; //GPIO_SPEED_FREQ_LOW;
  	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  	GPIO_InitStruct.Pin = GPIO_PIN_3;
  	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
}

#if 0
static void MX_CRC_Init(void)
{
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    ////Error_Handler();
  }
  __HAL_CRC_DR_RESET(&hcrc);
}
#endif

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
#if 0
void StartDefaultTask(void *argument)
{
  (void)argument;
#if 1
  unsigned long syserr;
#endif

  /* Infinite loop */
  for(;;)
  {
	  if (PWR_GetMode() == PWR_FULL)
	  {
		  /* green blinking LED as "still alive" */
		  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_SET);
		  osDelay(1000);
		  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_RESET);
		  osDelay(1000);

#if 1
		  syserr = SYS_GetError();
		  print_log(ITM_OUT, (const char *)"SYSERR: %lx\r\n", syserr);
		  print_log(ITM_OUT, (const char *)"IP address : %d.%d.%d.%d\r\n", (int)(gIPAddr & 0xFF), (int)((gIPAddr >> 8) & 0xFF), (int)((gIPAddr >> 16) & 0xFF), (int)((gIPAddr >> 24) & 0xFF));
#endif
	  }
	  else
	  {
		  __WFI();
	  }
  }
}
#else
void StartDefaultTask(void *argument)
{
  (void)argument;
  static int cnt = 0;
  for(;;)
  {
	  //this will be woken up every 1 ms, due to SysTick
	  if ( ! (GSysCfg.SysCfg & 0x1))
	  {
		  if (PWR_GetMode() == PWR_FULL)
		  {
			  /* green blinking LED as "still alive" */
			  if (cnt == 0)
				  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_SET);		//off
			  if (cnt == 1000)
				  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_RESET);		//on
			  cnt++;
			  if (cnt >= 1200)
				  cnt = 0;
		  }
	  }

	  //go to sleep, nothing to do, wait for an INT
	  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  }
}
#endif


unsigned char strRxBuf[UART_STR_SIZE];

void StartUARTTask(void *argument)
{
	(void)argument;
	int bytes;

#if 0
  /* initialize USB VCP for UART */
  USBD_Init(&USBD_Device, &CDC_Desc, 0);
  USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
  USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
  USBD_Start(&USBD_Device);
#endif

	/* initialize all drivers using RTOS features */
	SPI_Init();
	SPI_SetConfig(SYSCFG_getCfg(2));				//set the SPI bitrate

	if (I2CUser_Setup())
		SYS_SetError(SYS_ERR_SYS_INIT);
	else
		I2CUser_ClockSelect(SYSCFG_getCfg(6));		//set the I2C clock enum

#if 1
	/* it takes a while until USB is enumerated - wait for USB VCP */
	/* ATT: this would hang and wait for VCP UART! If we use just with network - don't wait */
	/* it is faster on UART tbis way: no prompt after fwreset and no welcome message */
	while (! CDC_isReady())
		osDelay(100);

	PRINT_VERSION;

	SendPrompt();
#endif

	/* Infinite loop */
	for(;;)
	{
		bytes = UART_getString(strRxBuf, sizeof(strRxBuf));
		if (bytes)
		{
#if 0
			UART_printString(strRxBuf, UART_OUT);
			UART_putChar('\n');
#else
			CMD_DEC_execute((char *)strRxBuf, UART_OUT);
#endif
			SendPrompt();

			UART_setLastString(strRxBuf);		/* set the last string, for ESC arrow up key */
		}
	}
}

void GPIOINTTask(void *argument)
{
  (void)argument;
  extern int picoc_INThandler(void);

  /* Infinite loop */
  for(;;)
  {
	  if (osSemaphoreAcquire(xSemaphoreGPIOINT, portMAX_DELAY) == osOK)
	  {
		  if (GDebug & DBG_VERBOSE)
			  UART_printString((unsigned char *)"*D: GPIO INT\r\n", UART_OUT);
		  	  //check if picoc is running and INT handler is registered

		  if ( ! picoc_INThandler())
			  //(otherwise) call command line "aint" command set
			  CMD_invokeAINT();

		  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	  }
  }
}

/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{
  extern struct netif gnetif;
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

  if (0)
  {
	  ip_addr_set_zero_ip4(&ipaddr);
	  ip_addr_set_zero_ip4(&netmask);
	  ip_addr_set_zero_ip4(&gw);
  }
  else
  {
	  IP_ADDR4(&ipaddr, IP_ADDR0 ,IP_ADDR1 , IP_ADDR2 , IP_ADDR3 );
	  IP_ADDR4(&netmask, NETMASK_ADDR0, NETMASK_ADDR1, NETMASK_ADDR2, NETMASK_ADDR3);
	  IP_ADDR4(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

	  gIPAddr = (unsigned long)ipaddr.addr;
  }

  /* add the network interface */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /*  Registers the default network interface. */
  netif_set_default(&gnetif);

  ethernet_link_status_updated(&gnetif);

  //TODO: we can start network just if link is up

#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&gnetif, ethernet_link_status_updated);
  EthernetHandle = osThreadNew(ethernet_link_thread, &gnetif, &EthernetThread_attributes);
#endif

#if LWIP_DHCP
  if ((GSysCfg.NetworkCfg & CFG_NET_STATIC) == 0)
	  /* Start DHCPClient */
	  DHCPHandle = osThreadNew(DHCP_Thread, &gnetif, &DHCPThread_attributes);
  else
	  SYS_SetError(SYS_ERR_STATICIP);
#endif
}

void StartThread(void *argument)
{
  (void)argument;
  extern void MX_LWIP_Init(void);
#if 1
  err_t err;
#endif

#if 1
  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);			//it hangs here, task does not proceed, idle not running but UART is OK

  /* Initialize the LwIP stack */
  Netif_Config();
#else
  MX_LWIP_Init();
#endif

#if 1
  ////if (gCFGparams.NET_DHCP & CFG_NET_TFTP)
  {
	  err = tftp_init(&TFTPctx);
	  if (err != ERR_OK)
	  {
		  SYS_SetError(SYS_ERR_NETWORK);
	  }
  }
#endif

#if 1
  /* Initialize web server */
  http_server_netconn_init();
#endif

#if 0
  /* Initialize HTTPD port 8080 - REST-API */
  netcmd_taskCreate();
#endif

  for( ;; )
  {
    /* Delete the Init Thread */
	osThreadTerminate(osThreadGetId());
  }
}

#if 0
/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();

  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_5, GPIO_PIN_RESET);	/*turn and keep red LED on */
  SYS_SetError(SYS_ERR_SYSCFG);

  while (1)
  {
  }
}
#endif

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
