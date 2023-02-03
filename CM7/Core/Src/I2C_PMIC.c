/**
 * I2C_PMIC.c
 *
 *  Created on: Jul 10, 2022
 *      Author: Torsten Jaekel
 */

#include "I2C_PMIC.h"

#include "VCP_UART.h"								//for print_log()
#include "cmsis_os2.h"								//for osDelay()

I2C_HandleTypeDef  hi2cPMIC;

/* used during initialization */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  __HAL_RCC_GPIOB_CLK_ENABLE();

  if (hi2c->Instance == I2C1)
  {
    /** I2C1 GPIO Configuration - PMIC I2C1 (internal bus)
     * PB7     ------> I2C1_SDA
     * PB6     ------> I2C1_SCL
    */

	__HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;				//GPIO_NOPULL - does not hurt to use internal pull-up in parallel
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  }
  if (hi2c->Instance == I2C3)
  {
    /** I2C3 GPIO Configuration - user I2C - "Wire"
     * PH8     ------> I2C3_SDA
     * PH7     ------> I2C3_SCL
    */

	__HAL_RCC_GPIOH_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;				//GPIO_NOPULL - does not hurt to use internal pull-up in parallel
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    /* Peripheral clock enable */
    __HAL_RCC_I2C3_CLK_ENABLE();
  }
}

static int I2CPMIC_Init(void)
{
  hi2cPMIC.Instance = I2C1;

  /* 100 KHz */
  //hi2cPMIC.Init.Timing = 0x10709D9D;			//HSI: 99.661 KHz
  /* 400 KHz - FAST MODE */
  //hi2cPMIC.Init.Timing = 0x00604545;			//HSI: 400.641 KHz
  /* 1000 KHz - FAST MODE PLUS */
  hi2cPMIC.Init.Timing =  0x00301313;			//HSI: 1.048 MHz

  hi2cPMIC.Init.OwnAddress1 = 0;
  hi2cPMIC.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2cPMIC.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2cPMIC.Init.OwnAddress2 = 0;
  hi2cPMIC.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2cPMIC.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2cPMIC.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;
  if (HAL_I2C_Init(&hi2cPMIC) != HAL_OK)
  {
	  return 1;					//ERROR
  }

  /**
   * Configure filters
   */
  /* 100 KHz and 400 KHz */
  /**Configure Analog filter */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2cPMIC, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
	  return 1;					//ERROR
  }
  /**Configure Digital filter */
  //if (HAL_I2CEx_ConfigDigitalFilter(&hi2cPMIC, 0x10u) != HAL_OK)		//100 kHz
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2cPMIC, 0) != HAL_OK)			//400 KHz and 1000 KHz
  {
	  return 1;					//ERROR
  }

  /* if 1000 KHz */
  HAL_I2CEx_EnableFastModePlus(I2C_FASTMODEPLUS_I2C1);				//1000 KHz

  return 0;		//OK
}

int I2C_PMIC_Setup(void)
{
	/* PJ0 is output as PMIC_STDBY for STANDBY pin on PMIC, based on config: it is high active
	 * so, configure PJ0 and drive it low for RUN mode, we can try to drive high to enter STANDBY mode
	 * (standby voltages provided by PMIC)
	 * but it needs proper setting of PMIC registers, e.g. "regulator NOT enabled in standby"
	 * you can read the PMIC STATUS via register 0x67
	 */
	GPIO_InitTypeDef GPIO_InitStruct;

	__HAL_RCC_GPIOJ_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_0, GPIO_PIN_RESET);				//drive pin output low when enabled = RUN mode
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

	/* HSI Clock
	 * let's do it as early as possible (start of main()), before the SysClock is configured
	 * ATT: the HAL_Delay() will not work yet, therefore we use a NOP_loop
	 */
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
	PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_HSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
	    	return 1;    		//ERROR
	}

	return I2CPMIC_Init();
}

int I2C_PMIC_Read(uint16_t slaveAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	HAL_StatusTypeDef i2cerr;

		i2cerr = HAL_I2C_Master_Receive(&hi2cPMIC, slaveAddr, pData, num, I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;			//ERROR
		return err;
}

int I2C_PMIC_Write(uint16_t slaveAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	HAL_StatusTypeDef i2cerr;

		i2cerr = HAL_I2C_Master_Transmit(&hi2cPMIC, slaveAddr, pData, num, I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;			//ERROR
		return err;
}

int I2C_PMIC_MemRead(uint16_t slaveAddr, uint16_t regAddr, uint8_t *pData, uint16_t num)
{
	int err = 0;
	HAL_StatusTypeDef i2cerr;

		i2cerr = HAL_I2C_Mem_Read(&hi2cPMIC, slaveAddr, regAddr, I2C_MEMADD_SIZE_8BIT, pData, num, I2C_TIMEOUT);
		if (i2cerr == HAL_OK)
			err = 0;
		else
			err = 1;			//ERROR
		return err;
}

int I2C_PMIC_Initialize(void)
{
	//check if we can read the PMIC ChipID (should be 0x7C)
	uint8_t data[2];
	int i;
	int err;

	err = I2C_PMIC_MemRead(PMIC_I2C_SLAVE_ADDR, 0, data, 1);
	if (err)
		return err;				//ERROR

	if (data[0] != 0x7C)
		return 0;

	//initialize the PMIC registers:
	// LDO2 to 1.8V
	  data[0]=0x4F;
	  data[1]=0x0;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  /* ATTENTION: if I write this register - the I2C is dead, read ChipID does not work anymore
	   * write this register as the last one!
	   */
#if 0
	  data[0]=0x50;
	  data[1]=0xF;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
#endif

	  // LDO1 to 1.0V
	  data[0]=0x4c;
	  data[1]=0x5;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x4d;
	  data[1]=0xF;			//was 0x3
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  // LDO3 to 1.2V
	  data[0]=0x52;
	  data[1]=0x9;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x53;
	  data[1]=0xF;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x58;
	  data[1]=0x3;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  for (i = 0; i < 100000; i++)
		  __NOP();

	  //charger LED off - duty cycle
	  data[0]=0x9C;
	  data[1]=(1 << 7);
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  // Disable charger led
	  data[0]=0x9E;
	  data[1]=(1 << 5);
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  for (i = 0; i < 100000; i++)
		  __NOP();

	  // SW3: set 2A as current limit
	  // Helps keeping the rail up at wifi startup
	  data[0]=0x42;
	  data[1]=(2);
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  for (i = 0; i < 100000; i++)
		  __NOP();

	  // Change VBUS INPUT CURRENT LIMIT to 1.5A
	  data[0]=0x94;
	  data[1]=(20 << 3);		//0xA0
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  // SW2 to 3.3V (SW2_VOLT) !!!
	  // this changes to 3.3V for MCU and board!
	  /* we can set here 3.3V, but:
	   * it has to be done before PMIC has started, when PMIC has entered RUN state,
	   * any write does not change anymore, so, it must be done as the very first initialization,
	   * resetting MCU does not help: PMIC is not reset, just via power cycle, so, this code
	   * must be the first done after power cycle, not later again
	   */
	  data[0]=0x38;
	  data[1]=0x7;			//7 = 3.3V, was 0x6 = 3.0V, 0x5 = 2.5V
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
	  /* this is the MCU VRef voltage! */

	  data[0]=0x39;
	  data[1]=0x5;			//6 = 3.0V, 7 = 3.3V, 5 = 2.5 - but I do not see it
	  /*
	   * it depends on SW1 config!:
	   * if SW1 0x32 is 0x06 (3.0V) : in STANDBY we see on SW2 2.7V
	   * if SW1 0x32 is 0x07 (3.0V) : in STANDBY we see correct SW2 3.0V
	   */
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x3A;
	  data[1]=0x5;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x3B;
	  data[1]=0x1;				//was 0xF - 0x1 to change to STANDBY voltage!
	  /* this allows to toggle via STANDBY (PJ0 = high) to standby voltage 3.0V - IT WORKS */
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
	  /* this above works, but it disables voltage completely on external 3V3 signals,
	   * but it does not lower the internal 3.1 or 3.3V
	   */

	  // SW1 to 3.3V (SW1_VOLT) !!!
	  /* no idea if RUN vs. STANDBY works on SW1 (does not look like)
	   * this is SW1 as VCAP - MCU digital voltage supply
	   */
	  data[0]=0x32;
	  data[1]=0x7;			//7 = 3.3V, was 0x6 = 3.0V - works to change here
	  /* 6 = 3.0V for MCU, VCAP, but it influences the SW2 STANDBY voltage: down to 2.7V! */
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x33;
	  data[1]=0x5;			//but STANDBY mode is not entered - why not?
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x34;
	  data[1]=0x5;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x35;
	  data[1]=0x1;				//was 0xF - 0x1 to change to STANDBY voltage - does NOT work
	  /* ATT: if we set this to 0x1: on STANDBY the SW2 is just 2.0V - USB-C UART dead, but core still running
	   * the SW1, VCAP, MCU digital voltage goes to 2.1V
	   * WHY?
	   */
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

//#if NOT_EFFECTIVE_DUE_TO_OTP
	  // SW3 to 3.3V (SW3_VOLT) !!! does not seem to have effect
	  // I2C pull-ups remain on 3.1V
	  /* Reason: SW3 gets all the parameters only from OTP, not via register write,
	   * so, we cannot change: it remains on SW3_VOLT as 0xD = 3.1V (all voltages use the same parameter)
	   * it looks like this voltage we see on I2C pull-ups (measure SDA or SCL voltage)
	  * SW3 does not have DVFS! (no difference RUN, STANDBY, SLEEP - all the same
	  */
	  data[0]=0x3E;
	  data[1]=0xF;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x3F;
	  data[1]=0xE;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x40;
	  data[1]=0xE;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	  data[0]=0x41;
	  data[1]=0xF;			//was 0xF
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
//#endif

#if 1
	  for (i = 0; i < 100000; i++)
		  __NOP();
	  data[0]=0x50;
	  data[1]=0xF;
	  I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
#endif

	  /* Remark:
	   * it looks like: we can write the register again after done once already
	   * on power-up, but no effects.
	   * We need a power cycle of the board to make changes done here effective!
	   */

	  return 0;		//OK
}

//does not have an effect! PMIC wants to change just with a power cycle!
void I2C_PMIC_LowVoltage(void)
{
	uint8_t data[2];

	data[0]=0x38;
	data[1]=0x4;			//7 = 3.3V, was 0x6 = 3.0V, 5 = 2.5V
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	data[0]=0x32;
	data[1]=0x4;			//7 = 3.3V, was 0x6 = 3.0V, 5 = 2.5V
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
}

void I2C_PMIC_HighVoltage(void)
{
	uint8_t data[2];

	data[0]=0x38;
	data[1]=0x7;			//7 = 3.3V, was 0x6 = 3.0V
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	data[0]=0x32;
	data[1]=0x6;			//7 = 3.3V, was 0x6 = 3.0V, 5 = 2.5V
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
}

void I2C_PMIC_ReadOTP(void)
{

	uint8_t data[2];
	int i;

	//enable OTP reading:
	data[0]=0x6F;				//KEY1
	data[1]=0x15;
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	data[0]=0x9F;				//KEY2
	data[1]=0x50;
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	data[0]=0xDF;				//TEST_REG_KEY3
	data[1]=0xAB;
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	data[0]=0x6B;				//RC REQ 16MHz
	data[1]=0x01;
	I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));

	//OTP is indirect read, via register 0xC4 = FMRADDR and 0xC5 = FRMDATA
	//OTP is register address 0x1C...0x36
	for (i = 0x1C; i < 0x36; i++)	//OTP register address 0x1C...0x36
	{
		data[0]=0xC4;				//FMRADDR
		data[1]=(uint8_t)i;
		I2C_PMIC_Write(PMIC_I2C_SLAVE_ADDR, data, sizeof(data));
		//we have to wait a bit for the doing read and result ready
		osDelay(2);

		I2C_PMIC_MemRead(PMIC_I2C_SLAVE_ADDR, 0xC5, data, 1);	//read from FRMDATA

		if ((i % 16) == 0)
			print_log(UART_OUT, "\r\n");
		print_log(UART_OUT, "%02x ", data[0]);

	}
	print_log(UART_OUT, "\r\n");
}
