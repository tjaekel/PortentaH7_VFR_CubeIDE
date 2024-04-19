/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Audio Python module.
 */
#ifdef TARGET_PORTENTA_H7

#include <stdio.h>
#include "stm32h7xx_hal.h"
#include "pdm2pcm_glo.h"
#include "audio.h"
#include "stdbool.h"

#include "VCP_UART.h"

#include "cmsis_os.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/err.h"

static CRC_HandleTypeDef hcrc;
static SAI_HandleTypeDef hsai;
static DMA_HandleTypeDef hdma_sai_rx;

static int g_i_channels = AUDIO_SAI_NBR_CHANNELS;
static int g_o_channels = AUDIO_SAI_NBR_CHANNELS;
static PDM_Filter_Handler_t  PDM_FilterHandler[2];
static PDM_Filter_Config_t   PDM_FilterConfig[2];

#define DMA_XFER_NONE   (0x00U)
#define DMA_XFER_HALF   (0x01U)
#define DMA_XFER_FULL   (0x04U)

#define AUDIO_FREQUENCY_192K          ((uint32_t)192000)
#define AUDIO_FREQUENCY_96K           ((uint32_t)96000)
#define AUDIO_FREQUENCY_64K           ((uint32_t)64000)
#define AUDIO_FREQUENCY_48K           ((uint32_t)48000)
#define AUDIO_FREQUENCY_44K           ((uint32_t)44100)
#define AUDIO_FREQUENCY_32K           ((uint32_t)32000)
#define AUDIO_FREQUENCY_22K           ((uint32_t)22050)
#define AUDIO_FREQUENCY_16K           ((uint32_t)16000)
#define AUDIO_FREQUENCY_11K           ((uint32_t)11025)
#define AUDIO_FREQUENCY_8K            ((uint32_t)8000)


static volatile uint32_t xfer_status = 0;

// BDMA can only access D3 SRAM4 memory.
//int8_t* PDM_BUFFER = (uint16_t*)0x38000000;
uint8_t PDM_BUFFER[PDM_BUFFER_SIZE] __attribute__ ((section(".pdm_buffer")));
#if 0
//we copy directly into USB buffer
int16_t PCM_BUFFER[PCM_BUFFER_SIZE] __attribute__ ((section(".dtcmram"))); //or: .pcm_buffer
volatile int16_t *g_pcmbuf = PCM_BUFFER;
#endif

//VBAN buffer: 2 * 5 of such frames plus each with the 28 byte header
//VBAN works better with 5 frames, minimum is 2 here
#define VBAN_NUM_FRAMES		5
#define VBAN_BUFFER_SIZE	(((48 * 2 * 2) * VBAN_NUM_FRAMES + 28) * 2) / 4		//as 32bit words
/* ATT: ETH does not work on RAM D3! */
uint32_t VBAN_BUFFER[VBAN_BUFFER_SIZE] /* __attribute__ ((section(".pcm_buffer"))) */;
int VBANBufIdx = 0;
uint32_t VBANSeqNum = 0;
uint8_t *VBANBufPtr = (uint8_t *)VBAN_BUFFER;
static ip_addr_t udpIPdest = {.addr = 0};
static struct netconn *connUDP;
int SendUDP(const unsigned char *b, unsigned int len);

/* from the PDM.cpp file */
void PDMIrqHandler(bool halftranfer);
uint32_t PDMgetBufferSize(void);
void PDMsetBufferSize(uint32_t size);

void audio_pendsv_callback(void);			//forward declaration

//-------------------------------------
uint32_t PDMgetBufferSize(void)
{
	return PDM_BUFFER_SIZE;
}

uint32_t PDMbufferSize = 0;
void PDMsetBufferSize(uint32_t size)
{
	PDMbufferSize = size;
}

int16_t *samples;

void __attribute__((section(".itcmram"))) PDMIrqHandler(bool halftransfer)
{
	audio_pendsv_callback();
}

/* get the MIC PDM buffer for USB streaming */
int16_t * __attribute__((section(".itcmram"))) PCM_GetBuffer(void)
{
	return samples;
}

//-------------------------------------

void __attribute__((section(".itcmram"))) AUDIO_SAI_DMA_IRQHandler(void)
{
    HAL_DMA_IRQHandler(hsai.hdmarx);
}

void __attribute__((section(".itcmram"))) HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    xfer_status |= DMA_XFER_HALF;
#ifdef CORE_CM7
    //necessary to do
    SCB_InvalidateDCache_by_Addr((uint32_t *)(&PDM_BUFFER[0]), PDM_BUFFER_SIZE / 2);
#endif
    PDMIrqHandler(true);
}

void __attribute__((section(".itcmram"))) HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    xfer_status |= DMA_XFER_FULL;
#ifdef CORE_CM7
    SCB_InvalidateDCache_by_Addr((uint32_t *)(&PDM_BUFFER[PDM_BUFFER_SIZE / 2]), PDM_BUFFER_SIZE / 2);
#endif
    PDMIrqHandler(false);
}

static uint32_t get_decimation_factor(uint32_t decimation)
{
    switch (decimation) {
        case 16:    return PDM_FILTER_DEC_FACTOR_16;
        case 24:    return PDM_FILTER_DEC_FACTOR_24;
        case 32:    return PDM_FILTER_DEC_FACTOR_32;
        case 48:    return PDM_FILTER_DEC_FACTOR_48;
        case 64:    return PDM_FILTER_DEC_FACTOR_64;
        case 80:    return PDM_FILTER_DEC_FACTOR_80;
        case 128:   return PDM_FILTER_DEC_FACTOR_128;
        default: return 0;
    }
}


static uint8_t get_mck_div(uint32_t frequency)
{
    switch(frequency){
        case AUDIO_FREQUENCY_8K:     return 48;  //SCK_x = sai_x_ker_ck/48 =  1024KHz  Ffs = SCK_x/64 =  16KHz stereo
        case AUDIO_FREQUENCY_11K:    return  8;  //SCK_x = sai_x_ker_ck/8  =  1411KHz  Ffs = SCK_x/64 =  22KHz stereo
        case AUDIO_FREQUENCY_16K:    return 24;  //SCK_x = sai_x_ker_ck/24 =  2048KHz  Ffs = SCK_x/64 =  32KHz stereo
        case AUDIO_FREQUENCY_22K:    return  4;  //SCK_x = sai_x_ker_ck/4  =  2822KHz  Ffs = SCK_x/64 =  44KHz stereo
        case AUDIO_FREQUENCY_32K:    return 12;  //SCK_x = sai_x_ker_ck/12 =  4096KHz  Ffs = SCK_x/64 =  64KHz stereo
        case AUDIO_FREQUENCY_44K:    return  2;  //SCK_x = sai_x_ker_ck/2  =  5644KHz  Ffs = SCK_x/64 =  88KHz stereo
        case AUDIO_FREQUENCY_48K:    return  8;  //SCK_x = sai_x_ker_ck/8  =  6144KHz  Ffs = SCK_x/64 =  96KHz stereo
        case AUDIO_FREQUENCY_64K:    return  6;  //SCK_x = sai_x_ker_ck/6  =  8192KHz  Ffs = SCK_x/64 = 128KHz stereo
        case AUDIO_FREQUENCY_96K:    return  4;  //SCK_x = sai_x_ker_ck/4  = 12288KHz  Ffs = SCK_x/64 = 192KHz stereo
        case AUDIO_FREQUENCY_192K:   return  2;  //SCK_x = sai_x_ker_ck/2  = 24576KHz  Ffs = SCK_x/64 = 384KHz stereo
        default:                     return  0;  //Same as 1
   }
}

// TODO: this needs to become a library function
bool isBoardRev2(void) {
#if 0
  uint32_t hse_speed;
  uint8_t* bootloader_data = (uint8_t*)(0x801F000);
  if (bootloader_data[0] != 0xA0 || bootloader_data[1] < 14) {
    hse_speed = 27000000;
  } else {
    hse_speed = bootloader_data[10] * 1000000;
  }
  return (hse_speed == 25000000);
#else
  return 1;
#endif
}

void sai_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    AUDIO_SAI_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOE_CLK_ENABLE();

    GPIO_InitStruct.Pin = AUDIO_SAI_CK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = AUDIO_SAI_CK_AF;
    HAL_GPIO_Init(AUDIO_SAI_CK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = AUDIO_SAI_D1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = AUDIO_SAI_D1_AF;
    HAL_GPIO_Init(AUDIO_SAI_D1_PORT, &GPIO_InitStruct);
}

int py_audio_init(size_t channels, uint32_t frequency, int gain_db, float highpass)
{
#if 0
    RCC_PeriphCLKInitTypeDef rcc_ex_clk_init_struct;

    HAL_RCCEx_GetPeriphCLKConfig(&rcc_ex_clk_init_struct);

    if((frequency == AUDIO_FREQUENCY_11K) || (frequency == AUDIO_FREQUENCY_22K) || (frequency == AUDIO_FREQUENCY_44K))
    {
        /* SAI clock config:
        PLL3_VCO Input = HSE_VALUE/PLL3M = 1 Mhz
        PLL3_VCO Output = PLL3_VCO Input * PLL3N = 429 Mhz
        SAI_CLK_x = PLL3_VCO Output/PLL3P = 429/38 = 11.289 Mhz */
        rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI4A;
        rcc_ex_clk_init_struct.Sai4AClockSelection = RCC_SAI4ACLKSOURCE_PLL3;
        rcc_ex_clk_init_struct.PLL3.PLL3P = 38;
        rcc_ex_clk_init_struct.PLL3.PLL3Q = 1;
        rcc_ex_clk_init_struct.PLL3.PLL3R = 1;
        rcc_ex_clk_init_struct.PLL3.PLL3N = 429;
        rcc_ex_clk_init_struct.PLL3.PLL3M = isBoardRev2() ? 25 : 27;

    } else {
        /* SAI clock config:
        PLL3_VCO Input = HSE_VALUE/PLL3M = 1 Mhz
        PLL3_VCO Output = PLL3_VCO Input * PLL3N = 344 Mhz
        sai_x_ker_ck = PLL3_VCO Output/PLL3P = 344/7 = 49.142 Mhz */
        rcc_ex_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SAI4A;
        rcc_ex_clk_init_struct.Sai4AClockSelection = RCC_SAI4ACLKSOURCE_PLL3;
        rcc_ex_clk_init_struct.PLL3.PLL3P = 7;
        rcc_ex_clk_init_struct.PLL3.PLL3Q = 1;
        rcc_ex_clk_init_struct.PLL3.PLL3R = 2;
        rcc_ex_clk_init_struct.PLL3.PLL3N = 344;
        rcc_ex_clk_init_struct.PLL3.PLL3M = isBoardRev2() ? 25 : 27;
    }

    HAL_RCCEx_PeriphCLKConfig(&rcc_ex_clk_init_struct);
#endif
    /* instead we do this in system_clock.s, as, for SAI4A:
     * PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC | RCC_PERIPHCLK_USB | RCC_PERIPHCLK_SPI2 | RCC_PERIPHCLK_QSPI | RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_SAI4A | RCC_PERIPHCLK_TIM;
     * PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48; //RCC_USBCLKSOURCE_PLL; 		//RCC_USBCLKSOURCE_PLL3;		//RCC_USBCLKSOURCE_HSI48;
     * PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_PLL2;	//RCC_QSPICLKSOURCE_PLL; //RCC_QSPICLKSOURCE_PLL2;
     * PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
     * PeriphClkInitStruct.Sai4AClockSelection = RCC_SAI4ACLKSOURCE_PLL3; //RCC_SAI4ACLKSOURCE_PLL; //RCC_SAI4ACLKSOURCE_PLL3;
     * PeriphClkInitStruct.Lptim345ClockSelection = RCC_LPTIM345CLKSOURCE_PCLK4;	//RCC_LPTIM345CLKSOURCE_D3PCLK1;	//RCC_LPTIM345CLKSOURCE_PLL2;
     * PeriphClkInitStruct.TIMPresSelection = RCC_TIMPRES_DESACTIVATED;	//RCC_TIMPRES_ACTIVATED;
     * if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
          return 0; // FAIL
     * }
     */

    sai_init();

    // Sanity checks
    if ((frequency != AUDIO_FREQUENCY_8K) &&
        (frequency != AUDIO_FREQUENCY_11K) &&
        (frequency != AUDIO_FREQUENCY_16K) &&
        (frequency != AUDIO_FREQUENCY_22K) &&
        (frequency != AUDIO_FREQUENCY_32K) &&
        (frequency != AUDIO_FREQUENCY_44K) &&
        (frequency != AUDIO_FREQUENCY_48K) &&
        (frequency != AUDIO_FREQUENCY_64K) &&
        (frequency != AUDIO_FREQUENCY_96K)){
        return 0;
    }

    if (channels != 1 && channels != 2) {
        return 0;
    } else {
        g_o_channels = channels;
        g_i_channels = AUDIO_SAI_NBR_CHANNELS;
    }

    uint32_t decimation_factor = 64; // Fixed decimation factor
    uint32_t decimation_factor_const = get_decimation_factor(decimation_factor);
    if (decimation_factor_const == 0) {
        return 0;
    }
    uint32_t samples_per_channel = (PDM_BUFFER_SIZE * 8) / (decimation_factor * g_i_channels * 2); // Half a transfer

    hsai.Instance                    = AUDIO_SAI;
    hsai.Init.Protocol               = SAI_FREE_PROTOCOL;
    hsai.Init.AudioMode              = SAI_MODEMASTER_RX;
    hsai.Init.DataSize               = (g_i_channels == 1) ? SAI_DATASIZE_8 : SAI_DATASIZE_16;
    //hsai.Init.DataSize               = (g_i_channels == 1) ? SAI_DATASIZE_16 : SAI_DATASIZE_32;
    hsai.Init.FirstBit               = SAI_FIRSTBIT_LSB;
    //hsai.Init.FirstBit               = SAI_FIRSTBIT_MSB;
    hsai.Init.ClockStrobing          = SAI_CLOCKSTROBING_RISINGEDGE;
    hsai.Init.Synchro                = SAI_ASYNCHRONOUS;
    hsai.Init.OutputDrive            = SAI_OUTPUTDRIVE_DISABLE;
    hsai.Init.NoDivider              = SAI_MASTERDIVIDER_DISABLE;
    hsai.Init.FIFOThreshold          = SAI_FIFOTHRESHOLD_1QF;
    hsai.Init.SynchroExt             = SAI_SYNCEXT_DISABLE;
    hsai.Init.AudioFrequency         = SAI_AUDIO_FREQUENCY_MCKDIV;
    hsai.Init.MonoStereoMode         = (g_i_channels == 1)  ? SAI_MONOMODE: SAI_STEREOMODE;
    hsai.Init.CompandingMode         = SAI_NOCOMPANDING;
    hsai.Init.TriState               = SAI_OUTPUT_RELEASED;

    // The master clock output (MCLK_x) is disabled and the SAI clock
    // is passed out to SCK_x bit clock. SCKx frequency = SAI_KER_CK / MCKDIV
    hsai.Init.Mckdiv                 = get_mck_div(frequency);
    hsai.Init.MckOutput              = SAI_MCK_OUTPUT_DISABLE;
    hsai.Init.MckOverSampling        = SAI_MCK_OVERSAMPLING_DISABLE;

    // Enable and configure PDM mode.
    hsai.Init.PdmInit.Activation     = ENABLE;
    hsai.Init.PdmInit.MicPairsNbr    = 1;
    hsai.Init.PdmInit.ClockEnable    = SAI_PDM_CLOCK1_ENABLE;

    hsai.FrameInit.FrameLength       = 16;
    //hsai.FrameInit.FrameLength       = 32;
    hsai.FrameInit.ActiveFrameLength = 1;
    hsai.FrameInit.FSDefinition      = SAI_FS_STARTFRAME;
    hsai.FrameInit.FSPolarity        = SAI_FS_ACTIVE_HIGH;
    hsai.FrameInit.FSOffset          = SAI_FS_FIRSTBIT;

    hsai.SlotInit.FirstBitOffset     = 0;
    hsai.SlotInit.SlotSize           = SAI_SLOTSIZE_DATASIZE;		//it is 0!
    //hsai.SlotInit.SlotSize           = SAI_SLOTSIZE_16B;
    hsai.SlotInit.SlotNumber         = (g_i_channels == 1) ? 2 : 1;
    hsai.SlotInit.SlotActive         = (g_i_channels == 1) ? (SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1) : SAI_SLOTACTIVE_0;

    // Initialize the SAI
    HAL_SAI_DeInit(&hsai);
    if (HAL_SAI_Init(&hsai) != HAL_OK) {
        return 0;
    }

    // Enable the DMA clock
    AUDIO_SAI_DMA_CLK_ENABLE();

    // Configure the SAI DMA
    hdma_sai_rx.Instance                 = AUDIO_SAI_DMA_STREAM;
    hdma_sai_rx.Init.Request             = AUDIO_SAI_DMA_REQUEST;
    hdma_sai_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_sai_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_sai_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_sai_rx.Init.PeriphDataAlignment = (g_i_channels == 1) ? DMA_PDATAALIGN_BYTE : DMA_PDATAALIGN_HALFWORD;
    hdma_sai_rx.Init.MemDataAlignment    = (g_i_channels == 1) ? DMA_MDATAALIGN_BYTE : DMA_MDATAALIGN_HALFWORD;
    hdma_sai_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_sai_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_sai_rx.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    hdma_sai_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    hdma_sai_rx.Init.MemBurst            = DMA_MBURST_SINGLE;
    hdma_sai_rx.Init.PeriphBurst         = DMA_MBURST_SINGLE;
    __HAL_LINKDMA(&hsai, hdmarx, hdma_sai_rx);

    // Initialize the DMA stream
    HAL_DMA_DeInit(&hdma_sai_rx);
    if (HAL_DMA_Init(&hdma_sai_rx) != HAL_OK) {
        return 0;
    }

    // Configure and enable SAI DMA IRQ Channel
    HAL_NVIC_SetPriority(AUDIO_SAI_DMA_IRQ, AUDIO_IN_IRQ_PREPRIO, 0);
    HAL_NVIC_EnableIRQ(AUDIO_SAI_DMA_IRQ);

    // Init CRC for the PDM library
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    if (HAL_CRC_Init(&hcrc) != HAL_OK) {
        return 0;
    }
    __HAL_CRC_DR_RESET(&hcrc);

    // Configure PDM filters
    for (int i=0; i<g_i_channels; i++) {
        PDM_FilterHandler[i].bit_order  = PDM_FILTER_BIT_ORDER_MSB;

        PDM_FilterHandler[i].endianness = PDM_FILTER_ENDIANNESS_LE;
        //PDM_FilterHandler[i].endianness = PDM_FILTER_ENDIANNESS_BE;

        PDM_FilterHandler[i].high_pass_tap = (uint32_t) (highpass * 2147483647U); // coff * (2^31-1)
        PDM_FilterHandler[i].out_ptr_channels = g_o_channels;
        PDM_FilterHandler[i].in_ptr_channels  = g_i_channels;
        PDM_Filter_Init(&PDM_FilterHandler[i]);

        PDM_FilterConfig[i].mic_gain = gain_db;
        PDM_FilterConfig[i].output_samples_number = samples_per_channel;
        PDM_FilterConfig[i].decimation_factor = decimation_factor_const;
        PDM_Filter_setConfig(&PDM_FilterHandler[i], &PDM_FilterConfig[i]);
    }

    uint32_t min_buff_size = samples_per_channel * g_o_channels * sizeof(int16_t);
    uint32_t buff_size = PDMgetBufferSize();
    if(buff_size < min_buff_size) {
      PDMsetBufferSize(min_buff_size);
    }

#if 0
    //depends on (NOLOAD) used in linker script */
    memset(PDM_BUFFER, 0, sizeof(PDM_BUFFER));
    memset(PCM_BUFFER, 0, sizeof(PCM_BUFFER));
#endif

    //initialize the VBAN buffer headers (2x)
    //memset(VBAN_BUFFER, 0, VBAN_BUFFER_SIZE * 4);
    {
    	uint8_t *p = (uint8_t *)VBAN_BUFFER;
#define VBAN_OFF	(VBAN_NUM_FRAMES * 48 * 2 * 2 + 28)

    	*p = 'V'; *(p + VBAN_OFF) = 'V'; p++;
    	*p = 'B'; *(p + VBAN_OFF) = 'B'; p++;
    	*p = 'A'; *(p + VBAN_OFF) = 'A'; p++;
    	*p = 'N'; *(p + VBAN_OFF) = 'N'; p++;

    	*p = 0x03; *(p +VBAN_OFF) = 0x03; p++;

    	*p = VBAN_NUM_FRAMES*48-1; *(p + VBAN_OFF) = VBAN_NUM_FRAMES*48-1; p++;

    	*p = 1; *(p + VBAN_OFF) = 1; p++;

    	*p = 0x01; *(p + VBAN_OFF) = 0x01; p++;

    	*p = 'S'; *(p + VBAN_OFF) = 'S'; p++;
    	*p = 't'; *(p + VBAN_OFF) = 't'; p++;
    	*p = 'r'; *(p + VBAN_OFF) = 'r'; p++;
    	*p = 'e'; *(p + VBAN_OFF) = 'e'; p++;
    	*p = 'a'; *(p + VBAN_OFF) = 'a'; p++;
    	*p = 'm'; *(p + VBAN_OFF) = 'm'; p++;
    	*p = '1'; *(p + VBAN_OFF) = '1'; p++;
    	*p = '\0'; *(p + VBAN_OFF) = '\0'; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' '; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' '; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' '; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' '; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' '; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' '; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' '; p++;
    	*p = ' '; *(p + VBAN_OFF) = ' ';

    	//SeqNum later set
    }

    return 1;
}

void py_audio_gain_set(int gain_db)
{
    // Configure PDM filters
    for (int i=0; i<g_i_channels; i++) {
        PDM_FilterConfig[i].mic_gain = gain_db;
        //This will be called only after init so PDM_FilterConfig structure is already filled
        //PDM_FilterConfig[i].output_samples_number = samples_per_channel;
        //PDM_FilterConfig[i].decimation_factor = decimation_factor_const;
        PDM_Filter_setConfig(&PDM_FilterHandler[i], &PDM_FilterConfig[i]);
    }
}

void py_audio_deinit(void)
{
    // Stop SAI DMA.
    if (hdma_sai_rx.Instance != NULL) {
        HAL_SAI_DMAStop(&hsai);
    }

    // Disable IRQs
    HAL_NVIC_DisableIRQ(AUDIO_SAI_DMA_IRQ);

    if (hsai.Instance != NULL) {
        HAL_SAI_DeInit(&hsai);
        hsai.Instance = NULL;
    }

    if (hdma_sai_rx.Instance != NULL) {
        HAL_DMA_DeInit(&hdma_sai_rx);
        hdma_sai_rx.Instance = NULL;
    }

    g_i_channels = AUDIO_SAI_NBR_CHANNELS;
    g_o_channels = AUDIO_SAI_NBR_CHANNELS;
    //free(g_pcmbuf);
    //g_pcmbuf = NULL;
}

/* who is the caller? */
void __attribute__((section(".itcmram"))) audio_pendsv_callback(void)
{
	//TODO: do it directly to USB buffer
	extern int16_t *GET_USBBuffer(void);
	int16_t *p;
	p = GET_USBBuffer();

    // Check for half transfer complete.
    if ((xfer_status & DMA_XFER_HALF)) {
        // Clear buffer state.
        xfer_status &= ~(DMA_XFER_HALF);

        // Convert PDM samples to PCM
        for (int i=0; i</*g_i_channels*/1; i++) {
            //PDM_Filter(&((uint8_t*)PDM_BUFFER)[i], &((int16_t*)g_pcmbuf)[i], &PDM_FilterHandler[i]);
        	PDM_Filter(&((uint8_t*)PDM_BUFFER)[i], p + i, &PDM_FilterHandler[i]);
        }
#if 0
        samples = PCM_BUFFER;
#endif
#if TIMING_DEBUG
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
#endif
#ifdef CORE_CM7
        //not needed and it makes it too slow
        ////SCB_InvalidateDCache_by_Addr((uint32_t *)(&PCM_BUFFER[0]), PCM_BUFFER_SIZE / 2);
#endif
    } else if ((xfer_status & DMA_XFER_FULL)) { // Check for transfer complete.
        // Clear buffer state.
        xfer_status &= ~(DMA_XFER_FULL);

        // Convert PDM samples to PCM
        for (int i=0; i</*g_i_channels*/1; i++) {
            //PDM_Filter(&((uint8_t*)PDM_BUFFER)[PDM_BUFFER_SIZE / 2 + i], &((int16_t*)g_pcmbuf)[PCM_BUFFER_SIZE / 2 + i], &PDM_FilterHandler[i]);
        	PDM_Filter(&((uint8_t*)PDM_BUFFER)[PDM_BUFFER_SIZE / 2 + i], p + i, &PDM_FilterHandler[i]);
        }
#if 0
        samples = &PCM_BUFFER[PCM_BUFFER_SIZE / 2];
#endif
#if TIMING_DEBUG
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
#endif
#ifdef CORE_CM7
        ////SCB_InvalidateDCache_by_Addr((uint32_t *)(&PCM_BUFFER[PDM_BUFFER_SIZE / 2]), PCM_BUFFER_SIZE / 2);
#endif
    }

    //for VBAN - copy from USB buffer into VBAN double buffer
    if (udpIPdest.addr != 0)
    {
    	uint8_t *vbanp = (uint8_t *)VBAN_BUFFER;
    	if (VBANBufIdx >= VBAN_NUM_FRAMES)
    		vbanp += (28 + VBAN_NUM_FRAMES * PCM_BUFFER_SIZE) + 28 + (VBANBufIdx - VBAN_NUM_FRAMES) * PCM_BUFFER_SIZE;
    	else
    		vbanp += 28 + VBANBufIdx * PCM_BUFFER_SIZE;
    	memcpy(vbanp, p, PCM_BUFFER_SIZE);

    	//update VBAN SeqNum
    	if (VBANBufIdx == 0)
    	{
    		*(VBAN_BUFFER + (24/4)) = VBANSeqNum++;
    	}
    	else if (VBANBufIdx == VBAN_NUM_FRAMES)
    	{
    		*(VBAN_BUFFER + ((VBAN_NUM_FRAMES * PCM_BUFFER_SIZE + 28)/4) + (24/4)) = VBANSeqNum++;
    	}

    	//trigger the audio thread, every 2 frames, 2 ms
    	if (VBANBufIdx == (VBAN_NUM_FRAMES -1))
		{
    		//osSemaphoreRelease(xSemaphoreAudio);
    		VBANBufPtr = (uint8_t *)VBAN_BUFFER;
    		SendUDP(VBANBufPtr, (VBAN_BUFFER_SIZE * 4) / 2);
		}
    	else if (VBANBufIdx == (VBAN_NUM_FRAMES * 2 -1))
    	{
    		VBANBufPtr = (uint8_t *)( VBAN_BUFFER + ((VBAN_NUM_FRAMES * PCM_BUFFER_SIZE + 28)/4) );
    		SendUDP(VBANBufPtr, (VBAN_BUFFER_SIZE * 4) / 2);
    	}

    	//move the buffer index and wrap around
    	VBANBufIdx++;
    	if (VBANBufIdx >= (VBAN_NUM_FRAMES * 2))
    		VBANBufIdx = 0;
    }

}

void py_audio_start_streaming(void)
{
    // Clear DMA buffer status
    xfer_status &= DMA_XFER_NONE;

    // Start DMA transfer
    if (HAL_SAI_Receive_DMA(&hsai, (uint8_t*) PDM_BUFFER, PDM_BUFFER_SIZE / g_i_channels) != HAL_OK)
    {
    }
}

void py_audio_stop_streaming(void)
{
    // Stop SAI DMA.
    HAL_SAI_DMAStop(&hsai);
}

/* global function to init and start MIC audio */
void PDM_MIC_Init(unsigned long gain)
{
	if (gain)
	{
		if (py_audio_init(2, 48000, (int)gain, 0.9883f))
			py_audio_start_streaming();
	}
	else
	{
		py_audio_stop_streaming();
		py_audio_deinit();
	}
}

void SetUDPDest(unsigned long ipAddr)
{
	udpIPdest.addr = ipAddr;

	/* Create UDP connection handle */
	connUDP = netconn_new(NETCONN_UDP);
	netconn_set_nonblocking(connUDP, 0);
}

int __attribute__((section(".itcmram"))) SendUDP(const unsigned char *b, unsigned int len)
{
	err_t err;
	struct netbuf *nb;
	if (udpIPdest.addr != 0)
	{
		SCB_CleanDCache_by_Addr((uint32_t *)b, (int32_t)(((len + 32)/32) * 32));

		nb = netbuf_new();
		netbuf_ref(nb, b, len /*+ 1*/);

		err = netconn_sendto(connUDP, nb, &udpIPdest, 6980);
		if (err != ERR_OK)
		{
		}

		netbuf_delete(nb);

		return (int)err;
	}
	return -100;		/* special, to separate from err codes */
}

int __attribute__((section(".itcmram"))) SendTOFUDP(const unsigned char *b, unsigned int len)
{
	err_t err;
	struct netbuf *nb;
	if (udpIPdest.addr != 0)
	{
		SCB_CleanDCache_by_Addr((uint32_t *)b, (int32_t)(((len + 32)/32) * 32));

		nb = netbuf_new();
		netbuf_ref(nb, b, len /*+ 1*/);

		err = netconn_sendto(connUDP, nb, &udpIPdest, 8080);
		if (err != ERR_OK)
		{
		}

		HAL_GPIO_TogglePin(GPIOK, GPIO_PIN_6);

		netbuf_delete(nb);

		return (int)err;
	}
	return -100;		/* special, to separate from err codes */
}

#endif
