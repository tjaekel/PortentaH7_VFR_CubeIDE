/**
 * GPIO_user.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#include "GPIO_user.h"
#include "debug_sys.h"

/* table with all the user GPIO ports and pins */
const TGPIOUsr GPIOUsr[GPIO_NUM_PINS] = {
	/* GPIO_INT means: do not touch or configure, use as configured somewhere else */
	////{GPIOA, GPIO_PIN_9,  0, GPIO_OUTPUT | GPIO_INPUT, "PA9"},	//b0 D14, J1-33 - make it permanent GPIO out for "res"
	{GPIOA, GPIO_PIN_10, 0, GPIO_OUTPUT | GPIO_INPUT, "PA10"},		//b1 D13, J1-35
	{GPIOA, GPIO_PIN_8,  0, GPIO_OUTPUT | GPIO_INPUT, "PA8"},		//b2 D6,  J2-59
	{GPIOC, GPIO_PIN_6,  0, GPIO_OUTPUT | GPIO_INPUT, "PC6"},		//b3 D5,  J2-61
	{GPIOC, GPIO_PIN_7,  0, GPIO_OUTPUT | GPIO_INPUT, "PC7"},		//b4 D4,  J2-63
	{GPIOG, GPIO_PIN_7,  0, GPIO_OUTPUT | GPIO_INPUT, "PG7"},		//b5 D3,  J2-65
	{GPIOJ, GPIO_PIN_11, 0, GPIO_OUTPUT | GPIO_INPUT, "PJ11"},		//b6 D2,  J2-67
	{GPIOK, GPIO_PIN_1,  0, GPIO_OUTPUT | GPIO_INPUT, "PK1"},		//b7 D1,  J2-60
	{GPIOH, GPIO_PIN_15, 0, GPIO_OUTPUT | GPIO_INPUT, "PH15"},		//b8 D0,  J2-62
	{GPIOA, GPIO_PIN_4,  0, GPIO_OUTPUT | GPIO_INPUT, "PA4"},		//b9 D21, J2-78
	////{GPIOC, GPIO_PIN_3,  0, GPIO_OUTPUT | GPIO_INPUT, "PC3"},	//b10 CONFLICT: PC3 SPI1_COPI !! D20, J2-76
	////{GPIOC, GPIO_PIN_2,  0, GPIO_OUTPUT | GPIO_INPUT, "PC2"},	//b11 CONFLICT: PC2 SPI1_CIPO (J20-8) A4, J2-74
	//special: PAx_C, PCx_C need analog switch set - see switch statement in config!
	////{GPIOC, GPIO_PIN_3,  4, GPIO_OUTPUT | GPIO_INPUT, "PC3C"},	//b12 - CONFLICT: PC3 = SPI1_COPI (J2-42) - A3, J2-79
	////{GPIOC, GPIO_PIN_2,  3, GPIO_OUTPUT | GPIO_INPUT, "PC2C"},	//b13 - CONFLICT: PC2 SPI1_CIPO (J20-8) A4, J2-74
	////{GPIOA, GPIO_PIN_1,  2, GPIO_OUTPUT | GPIO_INPUT, "PA1C"},	//b14 - CONFLICT: ET_RCLK !!
	/* ATTENTION: PA1_C has just 3.08V on high and 326mV on low, not the same voltage as other GPIOs! - conflict with ET_ signal? */
	////{GPIOA, GPIO_PIN_0,  1, GPIO_OUTPUT | GPIO_INPUT, "PA0C"},	//b15 - b11 A0, J2-73
	{GPIOJ, GPIO_PIN_9,  0, GPIO_INT | GPIO_INPUT, "PJ9"},			//b10, UART3 RX silk screen (as MCU UART8_RX)
};

int GPIO_INTConfig(int level, int enable)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	__HAL_RCC_GPIOJ_CLK_ENABLE();

	GPIO_InitStruct.Pin   = GPIO_PIN_9;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

	if (level)
	{
		GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING | GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	}
	else
	{
		GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING | GPIO_MODE_INPUT;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
	}

	HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

	if (enable)
	{
		/* enable INT for GPIO - PJ9 */
		HAL_NVIC_SetPriority(EXTI9_5_IRQn, 13, 1);
		HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
	}
	else
	{
		HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	}

	return 0;
}

int GPIO_Config(unsigned long mask, unsigned long odMask)
{
	/* configure GPIO as Input, Output, optional with Open Drain for output, INT pins are not changed */
	GPIO_InitTypeDef GPIO_InitStruct;
	size_t i;
	unsigned long shiftMask = 0x1;

	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; //GPIO_SPEED_FREQ_LOW;

	for (i = 0; i < (sizeof(GPIOUsr) / sizeof(TGPIOUsr)); i++)
	{
		if (mask & shiftMask)
		{
			//1 is Output, odMask = 1 is Open Drain output
			if (odMask & shiftMask)
				GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
			else
				GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		}
		else
			GPIO_InitStruct.Mode = GPIO_MODE_INPUT;

		if ((!(GPIOUsr[i].GPIO_Mode & GPIO_INT)) && (!(GPIOUsr[i].GPIO_Mode & GPIO_NONE)))
		{
			/* do not configure INT GPIO and used pins - just if it is a regular INPUT or OUTPUT */
			GPIO_InitStruct.Pin = GPIOUsr[i].GPIO_Pin;
			HAL_GPIO_Init(GPIOUsr[i].GPIOx, &GPIO_InitStruct);
		}

		switch (GPIOUsr[i].GPIO_ASwitch)
		{
			/* connect direct analog ADC input pins PAx_C via analog switch */
			case 1: HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PA0, SYSCFG_SWITCH_PA0_CLOSE); break;	//PA0_C
			case 2: HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PA1, SYSCFG_SWITCH_PA1_CLOSE); break;	//PA1_C
			case 3: HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PC2, SYSCFG_SWITCH_PC2_CLOSE); break;	//PC2_C
			case 4: HAL_SYSCFG_AnalogSwitchConfig(SYSCFG_SWITCH_PC3, SYSCFG_SWITCH_PC3_CLOSE); break;	//PC3_C
			default: break;			//flag is 0 or anything else
		}
		shiftMask <<= 1;
	}

	//store in RTC registers
	{
		uint32_t *rtcBkpReg = (uint32_t *)&RTC_START_BKP_REG;
		//RTC->BKP7R = mask;
		//RTC->BKP8R = odMask;
		*(rtcBkpReg + 7) = mask;
		*(rtcBkpReg + 8) = odMask;
	}

	return 0;		//OK, no error
}

void GPIO_Set(unsigned long bits, unsigned long mask)
{
	/* set all GPIOs together with a value if configure as output */
	size_t i;
	unsigned long shiftMask = 0x1;

	if (mask == 0)
		mask = 0xFFFFFFFF;				//default: all bits affected

	for (i = 0; i < (sizeof(GPIOUsr) / sizeof(TGPIOUsr)); i++)
	{
		if ((GPIOUsr[i].GPIO_Mode & GPIO_OUTPUT) && (mask & shiftMask))
		{
			/* just if allowed to be used as output */
			if (bits & shiftMask)
				HAL_GPIO_WritePin(GPIOUsr[i].GPIOx, GPIOUsr[i].GPIO_Pin, GPIO_PIN_SET);
			else
				HAL_GPIO_WritePin(GPIOUsr[i].GPIOx, GPIOUsr[i].GPIO_Pin, GPIO_PIN_RESET);
		}
		shiftMask <<= 1;
	}

	RTC->BKP9R  = bits;
	RTC->BKP10R = mask;
}

unsigned long GPIO_Get(void)
{
	/* read all GPIO inputs */
	size_t i;
	unsigned long shiftMask = 0x1;
	unsigned long pinStates = 0L;

	for (i = 0; i < (sizeof(GPIOUsr) / sizeof(TGPIOUsr)); i++)
	{
		//if (GPIOUsr[i].GPIO_Mode & GPIO_INPUT)
		{
			/* just if possible as an input - we could read back also the outputs set */
			if (HAL_GPIO_ReadPin(GPIOUsr[i].GPIOx, GPIOUsr[i].GPIO_Pin) == GPIO_PIN_SET)
				pinStates |= shiftMask;
		}
		shiftMask <<= 1;
	}

	return pinStates;
}

void GPIO_BitSet(unsigned long bitNo, unsigned long val)
{
	if (bitNo > (sizeof(GPIOUsr) / sizeof(TGPIOUsr) - 1))
		return;

	if (GPIOUsr[bitNo].GPIO_Mode & GPIO_OUTPUT)
	{
		/* just if allowed to be used as output */
		if (val)
			HAL_GPIO_WritePin(GPIOUsr[bitNo].GPIOx, GPIOUsr[bitNo].GPIO_Pin, GPIO_PIN_SET);
		else
			HAL_GPIO_WritePin(GPIOUsr[bitNo].GPIOx, GPIOUsr[bitNo].GPIO_Pin, GPIO_PIN_RESET);
	}
}

unsigned long GPIO_BitGet(unsigned long bitNo)
{
	if (bitNo > (sizeof(GPIOUsr) / sizeof(TGPIOUsr) - 1))
		return 0L;		//like all zero

	return (unsigned long)HAL_GPIO_ReadPin(GPIOUsr[bitNo].GPIOx, GPIOUsr[bitNo].GPIO_Pin);
}

void GPIO_resConfig(void)
{
	//PA9 - permanent res out GPIO
	GPIO_InitTypeDef GPIO_InitStruct;

	//set high before we change to output:
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);

	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void GPIO_resLevel(unsigned long val)
{
	if (val)
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
	else
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
}
