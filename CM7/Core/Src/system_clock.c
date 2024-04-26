/**
 * system_clock.c
 *
 *  Created on: Jul 27, 2022
 *      Author: Torsten Jaekel
 */

#include "stm32h7xx.h"

/**
 * ATTENTION: define HSE_VALUE == 25000000
 * It looks like, even schematics shows 27 MHz clock as input on MCU (25 MHz is for ETH),
 * the MCU external clock is 25 MHz and not 27 MHz
 */
/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
     /* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00 /*!< Vector Table base offset field. 
                                   This value must be a multiple of 0x200. */

                                   // clock source is selected with CLOCK_SOURCE in json config
#define USE_PLL_HSE_EXTC     0x8  // Use external clock (ST Link MCO)
#define USE_PLL_HSE_XTAL     0x4  // Use external xtal (X3 on board - not provided by default)
#define USE_PLL_HSI          0x2  // Use HSI internal clock

#define CLOCK_SOURCE USE_PLL_HSE_XTAL
#define DEVICE_USBDEVICE     1

#if ( ((CLOCK_SOURCE) & USE_PLL_HSE_XTAL) || ((CLOCK_SOURCE) & USE_PLL_HSE_EXTC) )
int SetSysClock_PLL_HSE(uint8_t bypass, int lowspeed);
#endif /* ((CLOCK_SOURCE) & USE_PLL_HSE_XTAL) || ((CLOCK_SOURCE) & USE_PLL_HSE_EXTC) */

#if ((CLOCK_SOURCE) & USE_PLL_HSI)
uint8_t SetSysClock_PLL_HSI(void);
#endif /* ((CLOCK_SOURCE) & USE_PLL_HSI) */

/**
  * @brief  Configures the System clock source, PLL Multiplier and Divider factors,
  *               AHB/APBx prescalers and Flash settings
  * @note   This function should be called only once the RCC clock configuration
  *         is reset to the default reset state (done in SystemInit() function).
  * @param  None
  * @retval None
  */

#if ( ((CLOCK_SOURCE) & USE_PLL_HSE_XTAL) || ((CLOCK_SOURCE) & USE_PLL_HSE_EXTC) )
/******************************************************************************/
/*            PLL (clocked by HSE) used as System clock source                */
/******************************************************************************/
int SetSysClock_PLL_HSE(uint8_t bypass, int lowspeed)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    GPIO_InitTypeDef  gpio_osc_init_structure;

    /* Enable oscillator pin, PH1 */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_1, GPIO_PIN_SET);
    gpio_osc_init_structure.Pin = GPIO_PIN_1;
    gpio_osc_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_osc_init_structure.Pull = GPIO_NOPULL;
    gpio_osc_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOH, &gpio_osc_init_structure);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_Delay(10);		//wait for stable OSC clock

    //USB RESETB signal - ATT: configure and set high!
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, GPIO_PIN_SET);
    gpio_osc_init_structure.Pin = GPIO_PIN_4;
    gpio_osc_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_osc_init_structure.Pull = GPIO_NOPULL;
    gpio_osc_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOJ, &gpio_osc_init_structure);
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, GPIO_PIN_SET);

    // If we are reconfiguring the clock, select CSI as system clock source to allow modification of the PLL configuration
    if (__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_HSE) {
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_CSI;
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
            return 0;	//FAIL
        }
    }

    /* Supply configuration update enable */
////#if HSE_VALUE == 27000000
    ////HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_EXT);	//this creates trouble!
////#else
    HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_LDO);
////#endif
    /* Configure the main internal regulator output voltage */

    if (lowspeed) {
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
    }
    else {
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0 /* was: PWR_REGULATOR_VOLTAGE_SCALE1*/);
    }
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    if (bypass) {
        RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    }
    else {
        RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    }
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;

    /* Make sure that HSE_VALUE is set properly - it looks like Portenta H7 uses a 25 MHz XTAL, not 27 MHz as shown in schematics! */
#if HSE_VALUE == 25000000
    RCC_OscInitStruct.PLL.PLLM = 5;
    if (lowspeed) {
        RCC_OscInitStruct.PLL.PLLN = 24;		//if this gets too slow - the USB-C UART is messed up!
        										//20 : 50 MHz - OK, but problems on UART, 30 : 75 MHz, OK for UART
        										//25 : 62.5 MHz - OK, 24 : 60 MHz - OK
    }
    else {
        RCC_OscInitStruct.PLL.PLLN = 192;		//192 = 480 MHz - with a 25 MHz XTAL! 196 = 490 MHz, 200 = 500 MHz - over-clocked
        /* ATTENTION: when using Backup RAM: >450 MHz (value 180) fails: it crashes sometimes when accessing BackupRAM! */
    }
#endif

    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLQ = 7; //7;							//for FMC: SDRAM clock max. 143 MHz! 6 = 160 MHz, 7 = 137.14 MHz, plus SDMMC
    																//20 when USB is connected here
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;				//PLL input is 5 MHz (25 MHz / PLLM = 5)
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return 0; // FAIL
    }

    /* disable the internal oscillator - save power */
    __HAL_RCC_CSI_DISABLE();

    /* Select PLL as system clock source and configure bus clocks dividers */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
        RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_D3PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (lowspeed) {
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
            return 0;    // FAIL
        }
    }
    else {
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {	//XXXX 4 is default for this clock
            return 0;    // FAIL
        }
    }

    PeriphClkInitStruct.PLL2.PLL2M = 12;	//25 MHz / 12
      /*
       * Remark: we use: PLL2P = 4, PLL2Q = 4, PLL2M = 4
       * PLL2P is modified for SPI2 SPI clock setting! in combination with SPI divider
       */

    PeriphClkInitStruct.PLL2.PLL2N = 96;	//(25 MHz / 12) * 96 = 200 MHz
    PeriphClkInitStruct.PLL2.PLL2P = 4;		//50 MHz SPI123
    PeriphClkInitStruct.PLL2.PLL2Q = 4;		//50 MHz SPI45
    PeriphClkInitStruct.PLL2.PLL2R = 2;		//was 2: 100 MHz for SPI clock config, QSPI PLL2 or SDMMC
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;		//PLL2 input: 25 MHz / PLL2M = 12 = 2.08333 MHz
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    PeriphClkInitStruct.PLL2.PLL2FRACN = 0;

    //48 MHz for USB, also for PDM SAI4 MIC
    /*
     * we run SAI4 here on PLL3, we can trim factional multiplier, USB runs on PLL1 (PLL) for 48 MHz or here on PLL3
     */
    PeriphClkInitStruct.PLL3.PLL3M = 2;	//5;
    PeriphClkInitStruct.PLL3.PLL3N = 23; //192;
    PeriphClkInitStruct.PLL3.PLL3P = 6; //20;					//SAI4, PDM MIC., trimmed for 48KHz
    PeriphClkInitStruct.PLL3.PLL3Q = 6;	//20;					//USB 48 KHz
    PeriphClkInitStruct.PLL3.PLL3R = 4;	//20;
    PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_1;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN = 4860;					//we trim here the SAI to be in sync with USB 48 MHz - it depends on PC USB audio clock!

    /*
     * Remark: even we configure 48 MHz for USB - it is just used to the USB bit transfer, it is NOT the USB 1 KHz clock for frames!
     * So, the USB clock for audio transfer comes from the PC, external, independent clock reference.
     * The SAI4 should run "in sync" with USB: only possible via external clock feed, e.g. from USB SOF, or:
     * we trim the PLL3, where SAI is running and we trim the fractional multiplier. - seems to work, but
     * we would need an "adaptive clock" setting (set fractional based on a "drift indication", e.g. using SYSCLOCKTICK value,
     * assuming we can set fractional w/o to stop the PLL!
     */

    if (lowspeed)
    {
    	 PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    	 //enable also RTC: we use the BKPxx registers
    	 PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_ADC;
    	 PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48; //RCC_USBCLKSOURCE_PLL; 		//RCC_USBCLKSOURCE_PLL3;		//RCC_USBCLKSOURCE_HSI48;
    	 PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
    }
    else
    {
    	/* make sure anyway, to have just SPIs on PLL2 ! */
    	PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL2;
    	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    	////PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2;
    	PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2; //RCC_FMCCLKSOURCE_PLL;
    	PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL2;

    	//enable also RTC: we use the BKPxx registers
    	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_SPI2 | RCC_PERIPHCLK_QSPI | RCC_PERIPHCLK_ADC /*| RCC_PERIPHCLK_SAI4A*/ | RCC_PERIPHCLK_TIM;
    	PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;	//RCC_USBCLKSOURCE_HSI48; //RCC_USBCLKSOURCE_PLL; 		//RCC_USBCLKSOURCE_PLL3;		//RCC_USBCLKSOURCE_HSI48;
    	PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_PLL2;	//RCC_QSPICLKSOURCE_PLL; //RCC_QSPICLKSOURCE_PLL2;
    	PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
    	/* PeriphClkInitStruct.Sai4AClockSelection = RCC_SAI4ACLKSOURCE_PLL3; //RCC_SAI4ACLKSOURCE_PLL; //RCC_SAI4ACLKSOURCE_PLL3; */
    	PeriphClkInitStruct.Lptim345ClockSelection = RCC_LPTIM345CLKSOURCE_PCLK4;	//RCC_LPTIM345CLKSOURCE_D3PCLK1;	//RCC_LPTIM345CLKSOURCE_PLL2;
    	PeriphClkInitStruct.TIMPresSelection = RCC_TIMPRES_DESACTIVATED;	//RCC_TIMPRES_ACTIVATED;
    }
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
        return 0; // FAIL
    }

#if 1
    HAL_PWREx_EnableUSBVoltageDetector();
#endif

    // HAL_RCCEx_EnableBootCore(RCC_BOOT_C2);

    __HAL_RCC_CSI_ENABLE();

    __HAL_RCC_SYSCFG_CLK_ENABLE();

    if ( ! lowspeed)
    	HAL_EnableCompensationCell();

    /*enable the other domains and memories*/
    __HAL_RCC_D2SRAM1_CLK_ENABLE();
    __HAL_RCC_D2SRAM2_CLK_ENABLE();
    __HAL_RCC_D2SRAM3_CLK_ENABLE();

    /* backup RAM clock enable */
    ////__HAL_RCC_BKPRAM_CLK_ENABLE();

    /* enable RTC - we use BKPxx register for reset reason */
    __HAL_RCC_RTC_CLK_ENABLE();

    return 1; // OK
}
#endif /* ((CLOCK_SOURCE) & USE_PLL_HSE_XTAL) || ((CLOCK_SOURCE) & USE_PLL_HSE_EXTC) */

#if ((CLOCK_SOURCE) & USE_PLL_HSI)
/******************************************************************************/
/*            PLL (clocked by HSI) used as System clock source                */
/******************************************************************************/
uint8_t SetSysClock_PLL_HSI(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* Supply configuration update enable */
#if HSE_VALUE == 27000000
    HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_EXT);
#else
    HAL_PWREx_ConfigSupply(PWR_SMPS_1V8_SUPPLIES_LDO);
#endif

    /* The voltage scaling allows optimizing the power consumption when the device is
    clocked below the maximum system frequency, to update the voltage scaling value
    regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    // Enable HSI oscillator and activate PLL with HSI as source
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_CSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
    RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 100;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 10;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return 0; // FAIL
    }

    /* Select PLL as system clock source and configure  bus clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 | \
        RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        return 0; // FAIL
    }

    return 1; // OK
}
#endif /* ((CLOCK_SOURCE) & USE_PLL_HSI) */

#if 0
/* we do not use yet CM4 */
#if defined (CORE_CM4)
void HSEM2_IRQHandler(void)
{
    HAL_HSEM_IRQHandler();
}
#endif

#if defined (CORE_CM7)
void HSEM1_IRQHandler(void)
{
    HAL_HSEM_IRQHandler();
}
#endif
#endif
