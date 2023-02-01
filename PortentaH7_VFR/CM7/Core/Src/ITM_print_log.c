/*
 * ITM_print_log.c
 *
 *  Created on: Jun 5, 2018
 *      Author: Torsten Jaekel
 */

#include "ITM_print_log.h"

static int sITMEnabled = 0;

void ITM_enable(void)
{
#define SWO_BASE      	(0x5C003000UL)
#define SWTF_BASE      	(0x5C004000UL)
	  __IO GPIO_TypeDef *GPIOBRegs = (GPIO_TypeDef *)GPIOB;

	  uint32_t SWOSpeed = 2000000;		/* [Hz] we have 2 Mbps SWO speed in ST-Link SWO Viewer
	  	  	  	  	  	  	  	  	  	   if we select 480000000 Hz core clock */
	  ////uint32_t SWOPrescaler = (SystemCoreClock / SWOSpeed) - 1; /* divider value */
	  /* it looks like the value in SystemCoreClock is not upaded, not correct: we have changed
	     to 480 MHz - but using SystemCoreClock goes wrong in ST-LINK SWO viewer */
	  uint32_t SWOPrescaler = (480000000 / SWOSpeed) - 1;

	  //enable debug clocks
	  DBGMCU->CR = 0x00700000;										//enable debug clocks

	  //UNLOCK FUNNEL
	  //SWTF->LAR unlock
	  *((__IO uint32_t *)(SWTF_BASE + 0xFB0)) = 0xC5ACCE55;			//unlock SWTF

	  //SWO->LAR unlock
	  *((uint32_t *)(SWO_BASE + 0xFB0)) = 0xC5ACCE55;				//unlock SWO

	  //SWO divider setting
	  //This divider value (0x000000C7) corresponds to 400Mhz core clock
	  //SWO->CODR = PRESCALER[12:0]
	  *((__IO uint32_t *)(SWO_BASE + 0x010)) = SWOPrescaler;		//clock divider

	  //SWO set the protocol
	  //SWO->SPPR = PPROT[1:0] = NRZ
	  *((__IO uint32_t *)(SWO_BASE + 0x0F0)) = 0x00000002;			//set to NRZ

	  //Enable ITM input of SWO trace funnel, slave 0
	  //SWTF->CTRL bit 0 ENSO = Enable
	  *((__IO uint32_t *)(SWTF_BASE + 0x000)) |= 0x00000001;		//enable

	  //RCC_AHB4ENR enable GPIOB clock - maybe done already
	  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;

	  //Configure GPIOB_MODER pin 3 as AF
	  GPIOBRegs->MODER &= ~GPIO_MODER_MODE3;						//clear MODER3 bits
	  GPIOBRegs->MODER |=
			  GPIO_MODE_AF_PP << GPIO_OSPEEDR_OSPEED3_Pos;			//set MODER3 PIN3 bits as AF

	  //Configure GPIOB_OSPEEDR pin 3 Speed
	  GPIOBRegs->OSPEEDR &=
			  ~GPIO_OSPEEDR_OSPEED3;								//clear OSPEEDR3 bits
	  GPIOBRegs->OSPEEDR |=
			  GPIO_SPEED_FREQ_HIGH << GPIO_OSPEEDR_OSPEED3_Pos;		//set OSPEEDR3 PIN3 bits as High Speed

	  //Force AF0 for GPIOB_AFRL, AFR[0] for pin 3
	  GPIOBRegs->AFR[0] &= ~GPIO_AFRL_AFRL3;						//clear AFR2 PIN3 = 0 for AF0

	  sITMEnabled = 1;
}

void ITM_print(unsigned char *str)
{
	if ( ! sITMEnabled)
		return;

	while (*str)
	{
		ITM_SendChar(*str++);
	}
}
