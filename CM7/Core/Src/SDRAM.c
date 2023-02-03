/*
 * SDRAM.c
 *
 *  Created on: Aug 12, 2022
 *      Author: Torsten Jaekel
 */

#include "main.h"
#include "stm32h7xx_hal_sdram.h"
#include "SDRAM.h"

#if 0
#include <stm32h7xx_hal_mdma.h>
#endif

#define SDRAM_TIMEOUT                            ((uint32_t)0xFFFF)
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)

#ifdef FMC_SDRAM_BANK

static void sdram_init_seq(SDRAM_HandleTypeDef
        *hsdram, FMC_SDRAM_CommandTypeDef *command);
extern void __fatal_error(const char *msg);

#if 0
/* we do already in system_clock.c */
static HAL_StatusTypeDef FMC_SDRAM_Clock_Config(void)
{
  RCC_PeriphCLKInitTypeDef  RCC_PeriphCLKInitStruct;

  /* PLL2_VCO Input = HSE_VALUE/PLL2_M = 5 Mhz */
  /* PLL2_VCO Output = PLL2_VCO Input * PLL_N = 800 Mhz */
  /* FMC Kernel Clock = PLL2_VCO Output/PLL_R = 800/4 = 200 Mhz */
  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FMC;
  RCC_PeriphCLKInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_PLL2;
  RCC_PeriphCLKInitStruct.PLL2.PLL2RGE = RCC_PLL1VCIRANGE_2;
  RCC_PeriphCLKInitStruct.PLL2.PLL2M = 5;
  RCC_PeriphCLKInitStruct.PLL2.PLL2N = 160;
  RCC_PeriphCLKInitStruct.PLL2.PLL2FRACN = 0;
  RCC_PeriphCLKInitStruct.PLL2.PLL2P = 2;
  RCC_PeriphCLKInitStruct.PLL2.PLL2R = 6;
  RCC_PeriphCLKInitStruct.PLL2.PLL2Q = 4;
  RCC_PeriphCLKInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  return HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
}
#endif

int SDRAM_Init(void) {
    SDRAM_HandleTypeDef hsdram;
    FMC_SDRAM_TimingTypeDef SDRAM_Timing;
    FMC_SDRAM_CommandTypeDef command;
#if 0
    static MDMA_HandleTypeDef mdma_handle;
#endif
    GPIO_InitTypeDef gpio_init_structure;

#if 0
    FMC_SDRAM_Clock_Config();
#endif

    /* Enable FMC clock */
    __HAL_RCC_FMC_CLK_ENABLE();

    /* Enable chosen MDMAx clock */
    __HAL_RCC_MDMA_CLK_ENABLE();

    /* Enable GPIOs clock */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /** FMC GPIO Configuration
    PE1   ------> FMC_NBL1				OK
    PE0   ------> FMC_NBL0				OK
    PG15  ------> FMC_SDNCAS			OK
    PD0   ------> FMC_D2				OK
    PD1   ------> FMC_D3				OK
    PG8   ------> FMC_SDCLK				OK
    PF2   ------> FMC_A2				OK
    PF1   ------> FMC_A1				OK
    PF0   ------> FMC_A0				OK
    PG5   ------> FMC_BA1				OK
    PF3   ------> FMC_A3				OK
    PG4   ------> FMC_BA0				OK
    PG2   ------> FMC_A12				OK  we have 12 address lines, A12 is n.c.
    PF5   ------> FMC_A5				OK
    PF4   ------> FMC_A4				OK
    PH2   ------> FMC_SDCKE0			OK
    PE10  ------> FMC_D7				OK
    PH3   ------> FMC_SDNE0				OK	instead of PG3
    PH5   ------> FMC_SDNWE				OK
    PF13  ------> FMC_A7				OK
    PF14  ------> FMC_A8				OK
    PE9   ------> FMC_D6				OK
    PE11  ------> FMC_D8				OK
    PD15  ------> FMC_D1				OK
    PD14  ------> FMC_D0				OK
    PF12  ------> FMC_A6				OK
    PF15  ------> FMC_A9				OK
    PE12  ------> FMC_D9				OK
    PE15  ------> FMC_D12				OK
    PF11  ------> FMC_SDNRAS			OK
    PG0   ------> FMC_A10				OK
    PE8   ------> FMC_D5				OK
    PE13  ------> FMC_D10				OK
    PD10  ------> FMC_D15				OK
    PD9   ------> FMC_D14				OK
    PG1   ------> FMC_A11				OK
    PE7   ------> FMC_D4				OK
    PE14  ------> FMC_D11				OK
    PD8   ------> FMC_D13				OK
    */

    /* Common GPIO configuration */
    gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
    gpio_init_structure.Pull      = GPIO_PULLUP;
    gpio_init_structure.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_structure.Alternate = GPIO_AF12_FMC;

    /* GPIOD configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8| GPIO_PIN_9 | GPIO_PIN_10 |\
                                GPIO_PIN_14 | GPIO_PIN_15;

    HAL_GPIO_Init(GPIOD, &gpio_init_structure);

    /* GPIOE configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7| GPIO_PIN_8 | GPIO_PIN_9 |\
                                GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                                GPIO_PIN_15;

    HAL_GPIO_Init(GPIOE, &gpio_init_structure);

    /* GPIOF configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2| GPIO_PIN_3 | GPIO_PIN_4 |\
                                GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |\
                                GPIO_PIN_15;

    HAL_GPIO_Init(GPIOF, &gpio_init_structure);

    /* GPIOG configuration */
    gpio_init_structure.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 /*| GPIO_PIN_3 */|\
                                GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOG, &gpio_init_structure);

    /* GPIOH configuration */
    gpio_init_structure.Pin   = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOH, &gpio_init_structure);

#if 0
    /* we do not need here - if we use MDMA for memcpy (via DMA) - we initialize separately */
    /* Configure common MDMA parameters */
    mdma_handle.Init.Request = MDMA_REQUEST_SW;
    mdma_handle.Init.TransferTriggerMode = MDMA_BLOCK_TRANSFER;
    mdma_handle.Init.Priority = MDMA_PRIORITY_HIGH;
    mdma_handle.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    mdma_handle.Init.SourceInc = MDMA_SRC_INC_WORD;
    mdma_handle.Init.DestinationInc = MDMA_DEST_INC_WORD;
    mdma_handle.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
    mdma_handle.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
    mdma_handle.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    mdma_handle.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
    mdma_handle.Init.DestBurst = MDMA_DEST_BURST_SINGLE;
    mdma_handle.Init.BufferTransferLength = 128;
    mdma_handle.Init.SourceBlockAddressOffset = 0;
    mdma_handle.Init.DestBlockAddressOffset = 0;

    mdma_handle.Instance = MDMA_Channel1;

    /* Associate the DMA handle */
    __HAL_LINKDMA(&hsdram, hmdma, mdma_handle);

    /* Deinitialize the stream for new transfer */
    HAL_MDMA_DeInit(&mdma_handle);

    /* Configure the DMA stream */
    HAL_MDMA_Init(&mdma_handle);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(MDMA_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(MDMA_IRQn);
#endif

    /* SDRAM device configuration */
    hsdram.Instance = FMC_SDRAM_DEVICE;
    /* Timing configuration for 90 Mhz of SD clock frequency (180Mhz/2) */
    /* TMRD: 2 Clock cycles */
    SDRAM_Timing.LoadToActiveDelay    = MICROPY_HW_SDRAM_TIMING_TMRD;
    /* TXSR: min=70ns (6x11.90ns) */
    SDRAM_Timing.ExitSelfRefreshDelay = MICROPY_HW_SDRAM_TIMING_TXSR;
    /* TRAS */
    SDRAM_Timing.SelfRefreshTime      = MICROPY_HW_SDRAM_TIMING_TRAS;
    /* TRC */
    SDRAM_Timing.RowCycleDelay        = MICROPY_HW_SDRAM_TIMING_TRC;
    /* TWR */
    SDRAM_Timing.WriteRecoveryTime    = MICROPY_HW_SDRAM_TIMING_TWR;
    /* TRP */
    SDRAM_Timing.RPDelay              = MICROPY_HW_SDRAM_TIMING_TRP;
    /* TRCD */
    SDRAM_Timing.RCDDelay             = MICROPY_HW_SDRAM_TIMING_TRCD;

    #define _FMC_INIT(x, n) x ## _ ## n
    #define FMC_INIT(x, n) _FMC_INIT(x,  n)

    hsdram.Init.SDBank             = FMC_SDRAM_BANK;
    hsdram.Init.ColumnBitsNumber   = FMC_INIT(FMC_SDRAM_COLUMN_BITS_NUM, MICROPY_HW_SDRAM_COLUMN_BITS_NUM);
    hsdram.Init.RowBitsNumber      = FMC_INIT(FMC_SDRAM_ROW_BITS_NUM, MICROPY_HW_SDRAM_ROW_BITS_NUM);
    hsdram.Init.MemoryDataWidth    = FMC_INIT(FMC_SDRAM_MEM_BUS_WIDTH, MICROPY_HW_SDRAM_MEM_BUS_WIDTH);
    hsdram.Init.InternalBankNumber = FMC_INIT(FMC_SDRAM_INTERN_BANKS_NUM, MICROPY_HW_SDRAM_INTERN_BANKS_NUM);
    hsdram.Init.CASLatency         = FMC_INIT(FMC_SDRAM_CAS_LATENCY, MICROPY_HW_SDRAM_CAS_LATENCY);
    hsdram.Init.SDClockPeriod      = FMC_INIT(FMC_SDRAM_CLOCK_PERIOD, MICROPY_HW_SDRAM_CLOCK_PERIOD);
    hsdram.Init.ReadPipeDelay      = FMC_INIT(FMC_SDRAM_RPIPE_DELAY, MICROPY_HW_SDRAM_RPIPE_DELAY);
    hsdram.Init.ReadBurst          = (MICROPY_HW_SDRAM_RBURST) ? FMC_SDRAM_RBURST_ENABLE : FMC_SDRAM_RBURST_DISABLE;
    hsdram.Init.WriteProtection    = (MICROPY_HW_SDRAM_WRITE_PROTECTION) ? FMC_SDRAM_WRITE_PROTECTION_ENABLE : FMC_SDRAM_WRITE_PROTECTION_DISABLE;

    /* Initialize the SDRAM controller */
    if(HAL_SDRAM_Init(&hsdram, &SDRAM_Timing) != HAL_OK) {
        return 0;
    }

    sdram_init_seq(&hsdram, &command);
    return 1;
}

static void sdram_init_seq(SDRAM_HandleTypeDef
        *hsdram, FMC_SDRAM_CommandTypeDef *command)
{
    /* Program the SDRAM external device */
    __IO uint32_t tmpmrd =0;

    /* Step 3:  Configure a clock configuration enable command */
    command->CommandMode           = FMC_SDRAM_CMD_CLK_ENABLE;
    command->CommandTarget         = FMC_SDRAM_CMD_TARGET_BANK;
    command->AutoRefreshNumber     = 1;
    command->ModeRegisterDefinition = 0;

    /* Send the command */
    HAL_SDRAM_SendCommand(hsdram, command, 0x1000);

    /* Step 4: Insert 100 ms delay */
    HAL_Delay(100);

    /* Step 5: Configure a PALL (precharge all) command */
    command->CommandMode           = FMC_SDRAM_CMD_PALL;
    command->CommandTarget         = FMC_SDRAM_CMD_TARGET_BANK;
    command->AutoRefreshNumber     = 1;
    command->ModeRegisterDefinition = 0;

    /* Send the command */
    HAL_SDRAM_SendCommand(hsdram, command, 0x1000);

    /* Step 6 : Configure a Auto-Refresh command */
    command->CommandMode           = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    command->CommandTarget         = FMC_SDRAM_CMD_TARGET_BANK;
    command->AutoRefreshNumber     = MICROPY_HW_SDRAM_AUTOREFRESH_NUM;
    command->ModeRegisterDefinition = 0;

    /* Send the command */
    HAL_SDRAM_SendCommand(hsdram, command, 0x1000);

    /* Step 7: Program the external memory mode register */
    tmpmrd = (uint32_t)FMC_INIT(SDRAM_MODEREG_BURST_LENGTH, MICROPY_HW_SDRAM_BURST_LENGTH) |
        SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
        FMC_INIT(SDRAM_MODEREG_CAS_LATENCY, MICROPY_HW_SDRAM_CAS_LATENCY) |
        SDRAM_MODEREG_OPERATING_MODE_STANDARD |
        SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    command->CommandMode           = FMC_SDRAM_CMD_LOAD_MODE;
    command->CommandTarget         = FMC_SDRAM_CMD_TARGET_BANK;
    command->AutoRefreshNumber     = 1;
    command->ModeRegisterDefinition = tmpmrd;

    /* Send the command */
    HAL_SDRAM_SendCommand(hsdram, command, 0x1000);

    /* Step 8: Set the refresh rate counter
       RefreshRate = 64 ms / 8192 cyc = 7.8125 us/cyc
       RefreshCycles = 7.8125 us * 90 MHz = 703
       According to the formula on p.1665 of the reference manual,
       we also need to subtract 20 from the value, so the target
       refresh rate is 703 - 20 = 683.
     */
    #define REFRESH_COUNT (MICROPY_HW_SDRAM_REFRESH_RATE * 90000 / 8192 - 20)
    HAL_SDRAM_ProgramRefreshRate(hsdram, REFRESH_COUNT);
}

#endif // FMC_SDRAM_BANK
