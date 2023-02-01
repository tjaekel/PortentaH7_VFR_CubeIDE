/**
 * GPIO_user.h
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#ifndef GPIO_USER_H_
#define GPIO_USER_H_

#include <stm32h7xx.h>

/*
 * User GPIOs - have a table for the ports, pins
 */
#define GPIO_OUTPUT				0x0001
#define GPIO_INPUT				0x0002
#define	GPIO_INT				0x0004
#define GPIO_NONE				0x8000

typedef struct {
	GPIO_TypeDef* GPIOx;
	uint16_t GPIO_Pin;
	uint16_t GPIO_ASwitch;				//if set to >0: it is analog switch pin, e.g. PA0_C - change analog pin
	uint16_t GPIO_Mode;					//bit 0: OUTPUT, bit 1: INPUT, bit 2: INT - don't touch as GPIO config
	const char* Descr;					//string pointer, max. 4 characters, as name of GPIO, for ggpio -d
} TGPIOUsr;

/* Remark: actually: do not use analog pins and analog switch for digital GPIO pins! */

#define GPIO_NUM_PINS		10			//two not usable - it is ETH controller signal! also two conflicts for SPI signals!

extern const TGPIOUsr GPIOUsr[GPIO_NUM_PINS];		//we need the size here - otherwise compile error, unknown on external reference

int GPIO_Config(unsigned long mask, unsigned long odMask);
int GPIO_INTConfig(int level, int enable);
void GPIO_Set(unsigned long bits, unsigned long mask);
unsigned long GPIO_Get(void);

void GPIO_BitSet(unsigned long bitNo, unsigned long val);
unsigned long GPIO_BitGet(unsigned long bitNo);

void GPIO_resConfig(void);
void GPIO_resLevel(unsigned long val);

#endif /* GPIO_USER_H_ */
