/**
 * stm32h7xx_it.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#include "main.h"
#include "stm32h7xx_it.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

extern PCD_HandleTypeDef 	hpcd;
extern PCD_HandleTypeDef 	hpcd2;
extern ETH_HandleTypeDef 	heth;
extern TIM_HandleTypeDef	htim3;

#include "SPI.h"
/* SPI.h */
//extern SPI_HandleTypeDef 	hspi2;
//extern DMA_HandleTypeDef 	hdma_spi2rx;
//extern DMA_HandleTypeDef 	hdma_spi2tx;

////extern SD_HandleTypeDef hsd1;
extern SD_HandleTypeDef uSdHandle;
extern QSPI_HandleTypeDef hqspi;

extern osThreadId_t GPIOINTHandle;
extern osSemaphoreId xSemaphoreGPIOINT;
////extern osThreadId_t AudioHandle;

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
#if 0
volatile unsigned int stacked_r0;
volatile unsigned int stacked_r1;
volatile unsigned int stacked_r2;
volatile unsigned int stacked_r3;
volatile unsigned int stacked_r12;
volatile unsigned int stacked_lr;
volatile unsigned int stacked_pc;
volatile unsigned int stacked_psr;

void __USED HardFault_Handler_C(unsigned int *hardfault_args)
{
		stacked_r0 	= ((unsigned long) hardfault_args[0]);
		stacked_r1 	= ((unsigned long) hardfault_args[1]);
		stacked_r2 	= ((unsigned long) hardfault_args[2]);
		stacked_r3 	= ((unsigned long) hardfault_args[3]);

		stacked_r12 = ((unsigned long) hardfault_args[4]);
		stacked_lr 	= ((unsigned long) hardfault_args[5]);
		stacked_pc 	= ((unsigned long) hardfault_args[6]);
		stacked_psr = ((unsigned long) hardfault_args[7]);

		while (1) {
			__NOP();
		}
}
#endif

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void __attribute__((section(".itcmram"))) SysTick_Handler(void)
{
  HAL_IncTick();
#if (INCLUDE_xTaskGetSchedulerState == 1 )
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
#endif /* INCLUDE_xTaskGetSchedulerState */
	  xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState == 1 )
  }
#endif /* INCLUDE_xTaskGetSchedulerState */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

#ifdef USE_USB_FS
void __attribute__((section(".itcmram"))) OTG_FS_IRQHandler(void)
{
	HAL_PCD_IRQHandler(&hpcd2);
}
#endif

#ifdef USE_USB_HS
void __attribute__((section(".itcmram"))) OTG_HS_IRQHandler(void)
{
  HAL_PCD_IRQHandler(&hpcd);
}
#endif

/**
  * @brief  This function handles Ethernet interrupt request.
  * @param  None
  * @retval None
  */
void __attribute__((section(".itcmram"))) ETH_IRQHandler(void)
{
  HAL_ETH_IRQHandler(&heth);
}

/**
  * @brief This function handles DMA2 stream3 global interrupt.
  */
void __attribute__((section(".itcmram"))) DMA2_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_spi2tx);
}

void __attribute__((section(".itcmram"))) DMA2_Stream2_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_spi2rx);
}

void __attribute__((section(".itcmram"))) SPI2_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi2);
}

#if 0
void __attribute__((section(".itcmram"))) SDMMC2_IRQHandler(void)
{
  HAL_SD_IRQHandler(&uSdHandle);
}
#endif

void __attribute__((section(".itcmram"))) EXTI9_5_IRQHandler(void)
{
	HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);

	osSemaphoreRelease(xSemaphoreGPIOINT);
}

void __attribute__((section(".itcmram"))) QUADSPI_IRQHandler(void)
{
	HAL_QSPI_IRQHandler(&hqspi);
}

#if 0
//we do need a PWM TIM INT
void __attribute__((section(".itcmram"))) TIM3_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&htim3);
}
#endif

/*
 * defined in audio.c
void __attribute__((section(".itcmram"))) BDMA_Channel1_IRQHandler(void)
{
}
*/

