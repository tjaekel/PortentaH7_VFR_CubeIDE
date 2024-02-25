/**
 * I2C3_user.c
 *
 *  Created on: Jul 20, 2018
 *      Author: Torsten Jaekel
 */

#include "I2C3_user.h"

osSemaphoreId xSemaphoreI2C = NULL;
I2C_HandleTypeDef  hi2c3;
uint8_t I2CBuf[256];

static int I2CUser_Init(int n)
{
  hi2c3.Instance = I2C3;

  /* we run the I2C on HSI = 64 MHz, internal clock generator */

  switch (n)
  {
  /* 100 KHz - STANDARD */
  case 0 :
	  hi2c3.Init.Timing = 0x10709D9D;				//HSI: 99.661 KHz
	  break;
  /* 400 KHz - FAST MODE */
  case 1 :
	  hi2c3.Init.Timing = 0x00604545;				//HSI: 400.641 KHz
	  break;
  /* 1000 KHz - FAST MODE PLUS */
  default :
	  hi2c3.Init.Timing =  0x00301313;				//HSI: 1.066 MHz
  }

  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
	  return 1;					//ERROR
  }

  /**
   * Configure filters
   */
  /**Configure Analog filter */
  if (n < 2)
  {
	  /* 100 KHz and 400 KHz */
	  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
	  {
		  return 1;					//ERROR
	  }
  }

  /**Configure Digital filter */
  if (n == 0)
  {
	  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0x10u) != HAL_OK)		//100 kHz
	  {
		  return 1;					//ERROR
	  }
  }
  else
  {
	  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)			//400 KHz and 1000 KHz
	  {
		  return 1;					//ERROR
	  }
  }

  /* if 1000 KHz */
  if (n > 2)
	  HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);				//1000 KHz

  return 0;						//OK
}

int I2CUser_Setup(void)
{
	  xSemaphoreI2C   = osSemaphoreNew(1, 1, NULL);

	  if ( ! xSemaphoreI2C)
		  SYS_SetError(SYS_ERR_SYS_INIT);

	/* HSI Clock
	 * let's do it as early as possible (start of main()), before the SysClock is configured
	 * ATT: the HAL_Delay() will not work yet, therefore we use a NOP_loop
	 */
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C3;
	PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_HSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
	    	return 1;    		//ERROR
	}

	return I2CUser_Init(0);
}

int I2CUser_ClockSelect(int n)
{
	SYSCFG_setCfg(6, (unsigned long)n);

	return I2CUser_Init(n);
}

int I2CUser_Read(uint16_t slaveAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	HAL_StatusTypeDef i2cerr;

	if (osSemaphoreAcquire(xSemaphoreI2C, portMAX_DELAY /*1000*/) == osOK)
	{
		i2cerr = HAL_I2C_Master_Receive(&hi2c3, slaveAddr, pData, num, USER_I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;			//ERROR
		osSemaphoreRelease(xSemaphoreI2C);

		if (GDebug & DBG_I2C)
		{
			print_log(UART_OUT, "\r\n*D: I2C READ err: %x\r\n", i2cerr);
		}
		return err;
	}
	else
		return 1;				//ERROR
}

int I2CUser_Write(uint16_t slaveAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	HAL_StatusTypeDef i2cerr;

	if (osSemaphoreAcquire(xSemaphoreI2C, portMAX_DELAY /*1000*/) == osOK)
	{
		i2cerr = HAL_I2C_Master_Transmit(&hi2c3, slaveAddr, pData, num, USER_I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;
		osSemaphoreRelease(xSemaphoreI2C);

		if (GDebug & DBG_I2C)
		{
			print_log(UART_OUT, "\r\n*D: I2C WRITE err: %x\r\n", i2cerr);
		}
		return err;
	}
	else
		return 1;
}

int I2CUser_MemWrite(uint8_t *pData, uint16_t num)
{
	/* pData has as first entry the regAddr ! */
	uint16_t slaveAddr;
	slaveAddr = GSysCfg.I2CslaveAddr;
	return I2CUser_Write((uint16_t)slaveAddr << 1, pData, num);
}

int I2CUser_MemWrite2(uint8_t *pData, uint16_t num)
{
	/* pData has as first entry the regAddr ! */
	uint16_t slaveAddr;
	slaveAddr = GSysCfg.I2CslaveAddr2;
	return I2CUser_Write((uint16_t)slaveAddr << 1, pData, num);
}

int I2CUser_MemReadEx(uint16_t slaveAddr, uint16_t regAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	HAL_StatusTypeDef i2cerr;

	if (osSemaphoreAcquire(xSemaphoreI2C, portMAX_DELAY /*1000*/) == osOK)
	{
		i2cerr = HAL_I2C_Mem_Read(&hi2c3, slaveAddr, regAddr, I2C_MEMADD_SIZE_8BIT, pData, num, USER_I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;
		osSemaphoreRelease(xSemaphoreI2C);

		if ((GDebug & DBG_I2C) && (err == 1))
		{
			print_log(UART_OUT, "\r\n*D: I2C MEMREAD err: %x\r\n", i2cerr);
			print_log(UART_OUT, " ErrCode: %lx\r\n", hi2c3.ErrorCode);
			print_log(UART_OUT, " State  : %x\r\n", hi2c3.State);
		}
		return err;
	}
	else
		return 1;
}

int I2CUser_MemRead(uint16_t regAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	uint16_t slaveAddr;
	HAL_StatusTypeDef i2cerr;

	slaveAddr = GSysCfg.I2CslaveAddr;
	if (osSemaphoreAcquire(xSemaphoreI2C, portMAX_DELAY /*1000*/) == osOK)
	{
		i2cerr = HAL_I2C_Mem_Read(&hi2c3, (uint16_t)slaveAddr << 1, regAddr, I2C_MEMADD_SIZE_8BIT, pData, num, USER_I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;
		osSemaphoreRelease(xSemaphoreI2C);

		if ((GDebug & DBG_I2C) && (err == 1))
		{
			print_log(UART_OUT, "\r\n*D: I2C MEMREAD err: %x\r\n", i2cerr);
			print_log(UART_OUT, " ErrCode: %lx\r\n", hi2c3.ErrorCode);
			print_log(UART_OUT, " State  : %x\r\n", hi2c3.State);
		}
		return err;
	}
	else
		return 1;
}

int I2CUser_MemRead2(uint16_t regAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	uint16_t slaveAddr;
	HAL_StatusTypeDef i2cerr;

	slaveAddr = GSysCfg.I2CslaveAddr2;
	if (osSemaphoreAcquire(xSemaphoreI2C, portMAX_DELAY /*1000*/) == osOK)
	{
		i2cerr = HAL_I2C_Mem_Read(&hi2c3, (uint16_t)slaveAddr << 1, regAddr, I2C_MEMADD_SIZE_8BIT, pData, num, USER_I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;
		osSemaphoreRelease(xSemaphoreI2C);

		if ((GDebug & DBG_I2C) && (err == 1))
		{
			print_log(UART_OUT, "\r\n*D: I2C MEMREAD err: %x\r\n", i2cerr);
			print_log(UART_OUT, " ErrCode: %lx\r\n", hi2c3.ErrorCode);
			print_log(UART_OUT, " State  : %x\r\n", hi2c3.State);
		}
		return err;
	}
	else
		return 1;
}

/* command to program PMIC on another external board - used to recover other board */

int PMIC_Recover(void)
{
/* slave address PMIC */
#define PMIC_I2C_SLAVE_ADDR	(0x08 << 1)

	//check if we can read the PMIC ChipID (should be 0x7C)
	uint8_t data[2];
	int err;

	err = I2CUser_MemReadEx(PMIC_I2C_SLAVE_ADDR, 0, data, 1);
	if (err)
		return err;				//ERROR

	if (data[0] != 0x7C)
	{
		print_log(UART_OUT, "*E: external PMIC not found\r\n");
		return 0;				//ERROR - incorrect ChipID
	}

	//initialize the PMIC registers:
	// LDO2 to 1.8V
	  data[0]=0x4F;
	  data[1]=0x0;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  /* ATTENTION: if I write this register - the I2C is dead, read ChipID does not work anymore
	   * write this register as the last one!
	   */
#if 0
	  data[0]=0x50;
	  data[1]=0xF;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
#endif

	  // LDO1 to 1.0V
	  data[0]=0x4c;
	  data[1]=0x5;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x4d;
	  data[1]=0xF;		//was 0x3
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  // LDO3 to 1.2V
	  data[0]=0x52;
	  data[1]=0x9;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x53;
	  data[1]=0xF;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  osDelay(10);

	  //charger LED off - duty cycle
	  data[0]=0x9C;
	  data[1]=(1 << 7);		//0x80
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  // Disable charger led
	  data[0]=0x9E;
	  data[1]=(1 << 5);		//0x20
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  osDelay(10);

	  // SW3: set 2A as current limit
	  // Helps keeping the rail up at wifi startup
	  data[0]=0x42;
	  data[1]=2;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  osDelay(10);

	  // Change VBUS INPUT CURRENT LIMIT to 1.5A
	  data[0]=0x94;
	  data[1]=(20 << 3);		//0xA0
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  // SW2 to 3.3V (SW2_VOLT)
	  data[0]=0x3B;
	  data[1]=0xF;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  // SW1 to 3.0V (SW1_VOLT)
	  data[0]=0x35;
	  data[1]=0xF;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

#if 1
	  osDelay(10);
	  data[0]=0x50;
	  data[1]=0xF;
	  I2CUser_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
#endif

	  return 0;		//OK
}
