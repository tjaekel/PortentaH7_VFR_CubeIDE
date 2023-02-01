#pragma once
#ifndef __QSPI_H__
#define __QSPI_H__
#endif

#include <stm32h7xx.h>
#include <stm32h7xx_hal_qspi.h>
#include <core_cm7.h>							//cache maintenance

#include "cmsis_os2.h"
#include "cmd_dec.h"

extern void hex_dump(uint8_t *ptr, uint16_t len, int mode, EResultOut out);

/*********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Dummy cycles for STR read mode */
#define MT25TL01G_DUMMY_CYCLES_READ_QUAD      	8U
#define MT25TL01G_DUMMY_CYCLES_READ			  	6U		//not 8U !
/* Dummy cycles for DTR read mode */
#define MT25TL01G_DUMMY_CYCLES_READ_DTR       	6U
#define MT25TL01G_DUMMY_CYCLES_READ_QUAD_DTR  	8U

#define QSPI_START_PRESCALER					16
#define	QSPI_STR_PRESCALER						16
#define QSPI_DTR_PRESCALER						32

typedef struct {
	uint32_t FlashSize;          /*!< Size of the flash */
	uint32_t EraseSectorSize;    /*!< Size of sectors for the erase operation */
	uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
	uint32_t ProgPageSize;       /*!< Size of pages for the program operation */
	uint32_t ProgPagesNumber;    /*!< Number of pages for the program operation */
} MT25TL01G_Info;

/* MT25TL01G Component Error codes *********************************************/
#define MT25TL01G_OK                           0
#define MT25TL01G_ERROR_INIT                  -1
#define MT25TL01G_ERROR_COMMAND               -2
#define MT25TL01G_ERROR_TRANSMIT              -3
#define MT25TL01G_ERROR_RECEIVE               -4
#define MT25TL01G_ERROR_AUTOPOLLING           -5
#define MT25TL01G_ERROR_MEMORYMAPPED          -6
/**exported type **/


/******************MT25TL01G_Info_t**********************/
typedef struct
{
	uint32_t FlashSize;          /*!< Size of the flash */
	uint32_t EraseSectorSize;    /*!< Size of sectors for the erase operation */
	uint32_t EraseSectorsNumber; /*!< Number of sectors for the erase operation */
	uint32_t ProgPageSize;       /*!< Size of pages for the program operation */
	uint32_t ProgPagesNumber;    /*!< Number of pages for the program operation */
} MT25TL01G_Info_t;

/******************MT25TL01G_Transfer_t**********************/
typedef enum
{
	MT25TL01G_SPI_MODE = 0,                 /*!< 1-1-1 commands, Power on H/W default setting */
	MT25TL01G_SPI_2IO_MODE,                 /*!< 1-1-2, 1-2-2 read commands                   */
	MT25TL01G_SPI_4IO_MODE,                 /*!< 1-1-4, 1-4-4 read commands                   */
	MT25TL01G_QPI_MODE                      /*!< 4-4-4 commands                               */
} MT25TL01G_Interface_t;

/******************MT25TL01G_Transfer_t**********************/
typedef enum
{
	MT25TL01G_STR_TRANSFER = 0,             /* Single Transfer Rate */
	MT25TL01G_DTR_TRANSFER                  /* Double Transfer Rate */
} MT25TL01G_Transfer_t;

/******************MT25TL01G_DualFlash_t**********************/
typedef enum
{
	MT25TL01G_DUALFLASH_DISABLE = QSPI_DUALFLASH_DISABLE, /*!<  Single flash mode              */
	MT25TL01G_DUALFLASH_ENABLE = QSPI_DUALFLASH_ENABLE
} MT25TL01G_DualFlash_t;

/******************MT25TL01G_Erase_t**********************/
typedef enum
{
	MT25TL01G_ERASE_4K = 0,                 /*!< 4K size Sector erase */
	MT25TL01G_ERASE_32K,                    /*!< 32K size Block erase */
	MT25TL01G_ERASE_64K,                    /*!< 64K size Block erase */
	MT25TL01G_ERASE_CHIP                    /*!< Whole chip erase     */
} MT25TL01G_Erase_t;

#if 0
#define MT25TL01G_FLASH_SIZE                  0x8000000 /* 2 * 512 MBits => 2 * 64MBytes => 128MBytes*/
#define MT25TL01G_SECTOR_SIZE                 0x10000   /* 2 * 1024 sectors of 64KBytes */
#define MT25TL01G_SUBSECTOR_SIZE              0x1000    /* 2 * 16384 subsectors of 4kBytes */
#define MT25TL01G_PAGE_SIZE                   0x100     /* 2 * 262144 pages of 256 bytes */

#define MT25TL01G_DIE_ERASE_MAX_TIME          460000
#define MT25TL01G_SECTOR_ERASE_MAX_TIME       1000
#define MT25TL01G_SUBSECTOR_ERASE_MAX_TIME    400
#else
#define MT25TL01G_FLASH_SIZE                  0x1000000 /* 128Mbit -> 16MB */
#define MT25TL01G_SECTOR_SIZE                 0x4000    /* 0x100   sectors of 64KBytes */
#define MT25TL01G_SUBSECTOR_SIZE              0x1000    /* 0x1000  subsectors of 4kBytes = Page Erase Size */
#define MT25TL01G_PAGE_SIZE                   0x100     /* 0x10000 pages of 256 bytes = Page Size */

#define MT25TL01G_DIE_ERASE_MAX_TIME          460000
#define MT25TL01G_SECTOR_ERASE_MAX_TIME       1000
#define MT25TL01G_SUBSECTOR_ERASE_MAX_TIME    400
#endif

/**
 * @brief QSPI Device Commands
 */

 /* Reset Operations */
#define MT25TL01G_RESET_ENABLE_CMD                     0x66
#define MT25TL01G_RESET_MEMORY_CMD                     0x99

/* Identification Operations */
#define MT25TL01G_READ_ID_CMD                          0x9F
#define MT25TL01G_READ_ID_CMD2                         0x9F
#define MT25TL01G_MULTIPLE_IO_READ_ID_CMD              0xAF
#define MT25TL01G_READ_SERIAL_FLASH_DISCO_PARAM_CMD    0x5A

/* Read Operations */
#define MT25TL01G_READ_CMD                             0x03
#define MT25TL01G_READ_4_BYTE_ADDR_CMD                 0x13

#define MT25TL01G_FAST_READ_CMD                        0x0B
#define MT25TL01G_FAST_READ_DTR_CMD                    0x0D
#define MT25TL01G_FAST_READ_4_BYTE_ADDR_CMD            0x0C
#define MT25TL01G_FAST_READ_4_BYTE_DTR_CMD             0x0E

#define MT25TL01G_DUAL_OUT_FAST_READ_CMD               0x3B
#define MT25TL01G_DUAL_OUT_FAST_READ_DTR_CMD           0x3D
#define MT25TL01G_DUAL_OUT_FAST_READ_4_BYTE_ADDR_CMD   0x3C

#define MT25TL01G_DUAL_INOUT_FAST_READ_CMD             0xBB
#define MT25TL01G_DUAL_INOUT_FAST_READ_DTR_CMD         0xBD
#define MT25TL01G_DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD 0xBC

#define MT25TL01G_QUAD_OUT_FAST_READ_CMD               0x6B
#define MT25TL01G_QUAD_OUT_FAST_READ_DTR_CMD           0x6D
#define MT25TL01G_QUAD_OUT_FAST_READ_4_BYTE_ADDR_CMD   0x6C

#define MT25TL01G_QUAD_INOUT_FAST_READ_CMD             0xEB		//0xEB
#define MT25TL01G_QUAD_INOUT_FAST_READ_DTR_CMD         0xED
#define MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD 0xEC
#define MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_DTR_CMD  0xEE
/* Write Operations */
#define MT25TL01G_WRITE_ENABLE_CMD                     0x06		//OK
#define MT25TL01G_WRITE_DISABLE_CMD                    0x04		//OK

/* Register Operations */
#define MT25TL01G_READ_STATUS_REG_CMD                  0x05
#define MT25TL01G_READ_CONFIG_REG_CMD                  0x15		//read config register
#define MT25TL01G_WRITE_STATUS_REG_CMD                 0x01		//write status AND config register

#define MT25TL01G_READ_LOCK_REG_CMD                    0xE8
#define MT25TL01G_WRITE_LOCK_REG_CMD                   0xE5

#define MT25TL01G_READ_FLAG_STATUS_REG_CMD             0x70
#define MT25TL01G_CLEAR_FLAG_STATUS_REG_CMD            0x50

#define MT25TL01G_READ_NONVOL_CFG_REG_CMD              0xB5
#define MT25TL01G_WRITE_NONVOL_CFG_REG_CMD             0xB1

#define MT25TL01G_READ_VOL_CFG_REG_CMD                 0x85
#define MT25TL01G_WRITE_VOL_CFG_REG_CMD                0x81

#define MT25TL01G_READ_ENHANCED_VOL_CFG_REG_CMD        0x65
#define MT25TL01G_WRITE_ENHANCED_VOL_CFG_REG_CMD       0x61

#define MT25TL01G_READ_EXT_ADDR_REG_CMD                0xC8
#define MT25TL01G_WRITE_EXT_ADDR_REG_CMD               0xC5

/* Program Operations */
#define MT25TL01G_PAGE_PROG_CMD                        0x02
#define MT25TL01G_PAGE_PROG_4_BYTE_ADDR_CMD            0x12

#define MT25TL01G_DUAL_IN_FAST_PROG_CMD                0xA2
#define MT25TL01G_EXT_DUAL_IN_FAST_PROG_CMD            0xD2

#define MT25TL01G_QUAD_IN_FAST_PROG_CMD                0x32
#define MT25TL01G_EXT_QUAD_IN_FAST_PROG_CMD            0x38
#define MT25TL01G_QUAD_IN_FAST_PROG_4_BYTE_ADDR_CMD    0x34

/* Erase Operations */
#define MT25TL01G_SUBSECTOR_ERASE_CMD_4K               0x20
#define MT25TL01G_SUBSECTOR_ERASE_4_BYTE_ADDR_CMD_4K   0x21			//!!!!

#define MT25TL01G_SUBSECTOR_ERASE_CMD_32K              0x52

#define MT25TL01G_SECTOR_ERASE_CMD                     0xD8
#define MT25TL01G_SECTOR_ERASE_4_BYTE_ADDR_CMD         0xDC

#define MT25TL01G_DIE_ERASE_CMD                        0xC7			//or 0x60

#define MT25TL01G_PROG_ERASE_RESUME_CMD                0x7A
#define MT25TL01G_PROG_ERASE_SUSPEND_CMD               0x75

/* One-Time Programmable Operations */
#define MT25TL01G_READ_OTP_ARRAY_CMD                   0x4B
#define MT25TL01G_PROG_OTP_ARRAY_CMD                   0x42

/* 4-byte Address Mode Operations */
#define MT25TL01G_ENTER_4_BYTE_ADDR_MODE_CMD           0xB7
#define MT25TL01G_EXIT_4_BYTE_ADDR_MODE_CMD            0xE9

/* Quad Operations */
#define MT25TL01G_ENTER_QUAD_CMD                       0x35
#define MT25TL01G_EXIT_QUAD_CMD                        0xF5
#define MT25TL01G_ENTER_DEEP_POWER_DOWN                0xB9
#define MT25TL01G_RELEASE_FROM_DEEP_POWER_DOWN         0xAB

/*ADVANCED SECTOR PROTECTION Operations*/
#define MT25TL01G_READ_SECTOR_PROTECTION_CMD           0x2D
#define MT25TL01G_PROGRAM_SECTOR_PROTECTION            0x2C
#define MT25TL01G_READ_PASSWORD_CMD                    0x27
#define MT25TL01G_WRITE_PASSWORD_CMD                   0x28
#define MT25TL01G_UNLOCK_PASSWORD_CMD                  0x29
#define MT25TL01G_READ_GLOBAL_FREEZE_BIT               0xA7
#define MT25TL01G_READ_VOLATILE_LOCK_BITS              0xE8
#define MT25TL01G_WRITE_VOLATILE_LOCK_BITS             0xE5
	/*ADVANCED SECTOR PROTECTION Operations with 4-Byte Address*/
#define MT25TL01G_WRITE_4_BYTE_VOLATILE_LOCK_BITS      0xE1
#define MT25TL01G_READ_4_BYTE_VOLATILE_LOCK_BITS       0xE0
	/*One Time Programmable Operations */
#define MT25TL01G_READ_OTP_ARRAY                       0x4B
#define MT25TL01G_PROGRAM_OTP_ARRAY                    0x42

/**
  * @brief QSPI Register Bits
  */
  /* Status Register */
#define MT25TL01G_SR_WIP                      ((uint8_t)0x01)    /*!< Write in progress */
#define MT25TL01G_SR_WREN                     ((uint8_t)0x02)    /*!< Write enable latch */
#define MT25TL01G_SR_BLOCKPR                  ((uint8_t)0x5C)    /*!< Block protected against program and erase operations */
#define MT25TL01G_SR_PRBOTTOM                 ((uint8_t)0x20)    /*!< Protected memory area defined by BLOCKPR starts from top or bottom */
#define MT25TL01G_SR_SRWREN                   ((uint8_t)0x80)    /*!< Status register write enable/disable */

/* Non volatile Configuration Register */
#define MT25TL01G_NVCR_NBADDR                 ((uint16_t)0x0001) /*!< 3-bytes or 4-bytes addressing */
#define MT25TL01G_NVCR_SEGMENT                ((uint16_t)0x0002) /*!< Upper or lower 128Mb segment selected by default */
#define MT25TL01G_NVCR_DUAL                   ((uint16_t)0x0004) /*!< Dual I/O protocol */
#define MT25TL01G_NVCR_QUAB                   ((uint16_t)0x0008) /*!< Quad I/O protocol */
#define MT25TL01G_NVCR_RH                     ((uint16_t)0x0010) /*!< Reset/hold */
#define MT25TL01G_NVCR_DTRP                   ((uint16_t)0x0020) /*!< Double transfer rate protocol */
#define MT25TL01G_NVCR_ODS                    ((uint16_t)0x01C0) /*!< Output driver strength */
#define MT25TL01G_NVCR_XIP                    ((uint16_t)0x0E00) /*!< XIP mode at power-on reset */
#define MT25TL01G_NVCR_NB_DUMMY               ((uint16_t)0xF000) /*!< Number of dummy clock cycles */

/* Volatile Configuration Register */
#define MT25TL01G_VCR_WRAP                    ((uint8_t)0x03)    /*!< Wrap */
#define MT25TL01G_VCR_XIP                     ((uint8_t)0x08)    /*!< XIP */
#define MT25TL01G_VCR_NB_DUMMY                ((uint8_t)0xF0)    /*!< Number of dummy clock cycles */

/* Extended Address Register */
#define MT25TL01G_EAR_HIGHEST_SE              ((uint8_t)0x03)    /*!< Select the Highest 128Mb segment */
#define MT25TL01G_EAR_THIRD_SEG               ((uint8_t)0x02)    /*!< Select the Third 128Mb segment */
#define MT25TL01G_EAR_SECOND_SEG              ((uint8_t)0x01)    /*!< Select the Second 128Mb segment */
#define MT25TL01G_EAR_LOWEST_SEG              ((uint8_t)0x00)    /*!< Select the Lowest 128Mb segment (default) */

/* Enhanced Volatile Configuration Register */
#define MT25TL01G_EVCR_ODS                    ((uint8_t)0x07)    /*!< Output driver strength */
#define MT25TL01G_EVCR_RH                     ((uint8_t)0x10)    /*!< Reset/hold */
#define MT25TL01G_EVCR_DTRP                   ((uint8_t)0x20)    /*!< Double transfer rate protocol */
#define MT25TL01G_EVCR_DUAL                   ((uint8_t)0x40)    /*!< Dual I/O protocol */
#define MT25TL01G_EVCR_QUAD                   ((uint8_t)0x80)    /*!< Quad I/O protocol */

/* Flag Status Register */
#define MT25TL01G_FSR_NBADDR                  ((uint8_t)0x01)    /*!< 3-bytes or 4-bytes addressing */
#define MT25TL01G_FSR_PRERR                   ((uint8_t)0x02)    /*!< Protection error */
#define MT25TL01G_FSR_PGSUS                   ((uint8_t)0x04)    /*!< Program operation suspended */
#define MT25TL01G_FSR_PGERR                   ((uint8_t)0x10)    /*!< Program error */
#define MT25TL01G_FSR_ERERR                   ((uint8_t)0x20)    /*!< Erase error */
#define MT25TL01G_FSR_ERSUS                   ((uint8_t)0x40)    /*!< Erase operation suspended */
#define MT25TL01G_FSR_READY                   ((uint8_t)0x80)    /*!< Ready or command in progress */

int32_t MT25TL01G_GetFlashInfo(MT25TL01G_Info_t* pInfo);
#if 0
int32_t MT25TL01G_Enter4BytesAddressMode(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_Exit4BytesAddressMode(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
#endif
int32_t MT25TL01G_AutoPollingMemReady(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
/* Register/Setting Commands *************************************************/
int32_t MT25TL01G_WriteEnable(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_BlockErase(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint32_t BlockAddress, MT25TL01G_Erase_t BlockSize);
int32_t MT25TL01G_ChipErase(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_PageProgram(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
int32_t MT25TL01G_ReadSTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
int32_t MT25TL01G_ReadDTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
int32_t MT25TL01G_ReadStatusRegister(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* Value);
int32_t MT25TL01G_ReadConfigurationRegister(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* Value);
int32_t MT25TL01G_EnterQPIMode(QSPI_HandleTypeDef* Ctx);
int32_t MT25TL01G_ExitQPIMode(QSPI_HandleTypeDef* Ctx);

int32_t MT25TL01G_EnableMemoryMappedModeSTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_EnableMemoryMappedModeDTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_WriteDisable(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_ReadID(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* ID);

int32_t MT25TL01G_ResetMemory(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_ResetEnable(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);

#if 0
int32_t MT25TL01G_ReadSPBLockRegister(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* SPBRegister);
int32_t MT25TL01G_ReleaseFromDeepPowerDown(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_EnterDeepPowerDown(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_ProgEraseResume(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
int32_t MT25TL01G_ProgEraseSuspend(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode);
#endif

#define BSP_QSPI_Info_t                 MT25TL01G_Info_t
#define BSP_QSPI_Interface_t            MT25TL01G_Interface_t
#define BSP_QSPI_Transfer_t             MT25TL01G_Transfer_t
#define BSP_QSPI_DualFlash_t            MT25TL01G_DualFlash_t
#define BSP_QSPI_ODS_t                  MT25TL01G_ODS_t

typedef enum
{
#if 0
	BSP_QSPI_ERASE_8K = MT25TL01G_ERASE_4K,       	/*!< 8K size Sector erase = 2 x 4K as Dual flash mode is used for this board   */
	BSP_QSPI_ERASE_64K = MT25TL01G_ERASE_32K,      	/*!< 64K size Sector erase = 2 x 32K as Dual flash mode is used for this board */
	BSP_QSPI_ERASE_128K = MT25TL01G_ERASE_64K,      /*!< 128K size Sector erase = 2 x 64K as Dual mode is used for this board      */
	BSP_QSPI_ERASE_CHIP = MT25TL01G_ERASE_CHIP      /*!< Whole chip erase */
#else
	BSP_QSPI_ERASE_4K = MT25TL01G_ERASE_4K,       	/*!< 4K size Sector erase */
	BSP_QSPI_ERASE_32K = MT25TL01G_ERASE_32K,     	/*!< 64K size Sector erase */
	BSP_QSPI_ERASE_64K = MT25TL01G_ERASE_64K,      	/*!< 128K size Sector erase */
	BSP_QSPI_ERASE_CHIP = MT25TL01G_ERASE_CHIP      /*!< Whole chip erase */
#endif
} BSP_QSPI_Erase_t;

typedef enum
{
	QSPI_ACCESS_NONE = 0,          /*!<  Instance not initialized,             */
	QSPI_ACCESS_INDIRECT,          /*!<  Instance use indirect mode access     */
	QSPI_ACCESS_MMP                /*!<  Instance use Memory Mapped Mode read  */
} BSP_QSPI_Access_t;

typedef struct
{
	BSP_QSPI_Access_t    IsInitialized;   /*!<  Instance access Flash method     */
	BSP_QSPI_Interface_t InterfaceMode;   /*!<  Flash Interface mode of Instance */
	BSP_QSPI_Transfer_t  TransferRate;    /*!<  Flash Transfer mode of Instance  */
	uint32_t             DualFlashMode;   /*!<  Flash dual mode                  */
	uint32_t             IsMspCallbacksValid;
} BSP_QSPI_Ctx_t;

typedef struct
{
	BSP_QSPI_Interface_t        InterfaceMode;   /*!<  Current Flash Interface mode */
	BSP_QSPI_Transfer_t         TransferRate;    /*!<  Current Flash Transfer mode  */
	BSP_QSPI_DualFlash_t        DualFlashMode;   /*!<  Dual Flash mode              */
} BSP_QSPI_Init_t;

typedef struct
{
	uint32_t FlashSize;
	uint32_t ClockPrescaler;
	uint32_t SampleShifting;
	uint32_t DualFlashMode;
}MX_QSPI_Init_t;

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
typedef struct
{
	void(*pMspInitCb)(pQSPI_CallbackTypeDef);
	void(*pMspDeInitCb)(pQSPI_CallbackTypeDef);
}BSP_QSPI_Cb_t;
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */

/* QSPI instances number */
#define QSPI_INSTANCES_NUMBER         1U

/* Definition for QSPI modes */
#define BSP_QSPI_SPI_MODE            (BSP_QSPI_Interface_t)MT25TL01G_SPI_MODE      /* 1 Cmd Line, 1 Address Line and 1 Data Line    */
#define BSP_QSPI_SPI_1I2O_MODE       (BSP_QSPI_Interface_t)MT25TL01G_SPI_1I2O_MODE /* 1 Cmd Line, 1 Address Line and 2 Data Lines   */
#define BSP_QSPI_SPI_2IO_MODE        (BSP_QSPI_Interface_t)MT25TL01G_SPI_2IO_MODE  /* 1 Cmd Line, 2 Address Lines and 2 Data Lines  */
#define BSP_QSPI_SPI_1I4O_MODE       (BSP_QSPI_Interface_t)MT25TL01G_SPI_1I4O_MODE /* 1 Cmd Line, 1 Address Line and 4 Data Lines   */
#define BSP_QSPI_SPI_4IO_MODE        (BSP_QSPI_Interface_t)MT25TL01G_SPI_4IO_MODE  /* 1 Cmd Line, 4 Address Lines and 4 Data Lines  */
#define BSP_QSPI_DPI_MODE            (BSP_QSPI_Interface_t)MT25TL01G_DPI_MODE      /* 2 Cmd Lines, 2 Address Lines and 2 Data Lines */
#define BSP_QSPI_QPI_MODE            (BSP_QSPI_Interface_t)MT25TL01G_QPI_MODE      /* 4 Cmd Lines, 4 Address Lines and 4 Data Lines */

/* Definition for QSPI transfer rates */
#define BSP_QSPI_STR_TRANSFER        (BSP_QSPI_Transfer_t)MT25TL01G_STR_TRANSFER /* Single Transfer Rate */
#define BSP_QSPI_DTR_TRANSFER        (BSP_QSPI_Transfer_t)MT25TL01G_DTR_TRANSFER /* Double Transfer Rate */

/* Definition for QSPI dual flash mode */
#define BSP_QSPI_DUALFLASH_DISABLE   (BSP_QSPI_DualFlash_t)MT25TL01G_DUALFLASH_DISABLE   /* Dual flash mode enabled  */
/* Definition for QSPI Flash ID */
#define BSP_QSPI_FLASH_ID            QSPI_FLASH_ID_2

#if 0
/* QSPI block sizes for dual flash - NOT DEFINED ! */
#define BSP_QSPI_BLOCK_8K            MT25TL01G_SECTOR_4K
#define BSP_QSPI_BLOCK_64K           MT25TL01G_BLOCK_32K
#define BSP_QSPI_BLOCK_128K          MT25TL01G_BLOCK_64K
#endif

/* Definition for QSPI clock resources */
#define QSPI_CLK_ENABLE()              __HAL_RCC_QSPI_CLK_ENABLE()
#define QSPI_CLK_DISABLE()             __HAL_RCC_QSPI_CLK_DISABLE()
#define QSPI_CLK_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK1_CS_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK1_D0_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_BK1_D1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_BK1_D2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK1_D3_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_BK2_CS_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOC_CLK_ENABLE()	//__HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK2_D0_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOE_CLK_ENABLE()	//__HAL_RCC_GPIOH_CLK_ENABLE()
#define QSPI_BK2_D1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOE_CLK_ENABLE()	//__HAL_RCC_GPIOH_CLK_ENABLE()
#define QSPI_BK2_D2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOE_CLK_ENABLE()	//__HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK2_D3_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOE_CLK_ENABLE()	//__HAL_RCC_GPIOG_CLK_ENABLE()

#define QSPI_FORCE_RESET()         __HAL_RCC_QSPI_FORCE_RESET()
#define QSPI_RELEASE_RESET()       __HAL_RCC_QSPI_RELEASE_RESET()

/* Definition for QSPI Pins */
#define QSPI_CLK_PIN               GPIO_PIN_10
#define QSPI_CLK_GPIO_PORT         GPIOF
/* Bank 1 */
#define QSPI_BK1_CS_PIN            GPIO_PIN_6
#define QSPI_BK1_CS_GPIO_PORT      GPIOG
#define QSPI_BK1_D0_PIN            GPIO_PIN_11
#define QSPI_BK1_D0_GPIO_PORT      GPIOD
#define QSPI_BK1_D1_PIN            GPIO_PIN_12
#define QSPI_BK1_D1_GPIO_PORT      GPIOD
#define QSPI_BK1_D2_PIN            GPIO_PIN_7
#define QSPI_BK1_D2_GPIO_PORT      GPIOF
#define QSPI_BK1_D3_PIN            GPIO_PIN_13
#define QSPI_BK1_D3_GPIO_PORT      GPIOD

/* Bank 2 */
#define QSPI_BK2_CS_PIN            GPIO_PIN_11	//GPIO_PIN_6
#define QSPI_BK2_CS_GPIO_PORT      GPIOC		//GPIOG
#define QSPI_BK2_D0_PIN            GPIO_PIN_7	//GPIO_PIN_2
#define QSPI_BK2_D0_GPIO_PORT      GPIOE		//GPIOH
#define QSPI_BK2_D1_PIN            GPIO_PIN_8	//GPIO_PIN_3
#define QSPI_BK2_D1_GPIO_PORT      GPIOE		//GPIOH
#define QSPI_BK2_D2_PIN            GPIO_PIN_9	//GPIO_PIN_9
#define QSPI_BK2_D2_GPIO_PORT      GPIOE		//GPIOG
#define QSPI_BK2_D3_PIN            GPIO_PIN_10	//GPIO_PIN_14
#define QSPI_BK2_D3_GPIO_PORT      GPIOE		//GPIOG


/* MT25TL01G Micron memory */
/* Size of the flash */
////#define QSPI_FLASH_SIZE            26     		/* Address bus width to access whole memory space */
////#define QSPI_PAGE_SIZE             256

/* QSPI Base Address */
#define QSPI_BASE_ADDRESS          0x90000000

extern QSPI_HandleTypeDef hqspi;
extern BSP_QSPI_Ctx_t     QSPI_Ctx[QSPI_INSTANCES_NUMBER];

int32_t BSP_QSPI_Init(uint32_t Instance, BSP_QSPI_Init_t* Init);
int32_t BSP_QSPI_DeInit(uint32_t Instance);
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
int32_t BSP_QSPI_RegisterMspCallbacks(uint32_t Instance, BSP_QSPI_Cb_t* CallBacks);
int32_t BSP_QSPI_RegisterDefaultMspCallbacks(uint32_t Instance);
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */
int32_t BSP_QSPI_Read(uint32_t Instance, uint8_t* pData, uint32_t ReadAddr, uint32_t Size);
int32_t BSP_QSPI_Write(uint32_t Instance, uint8_t* pData, uint32_t WriteAddr, uint32_t Size);
int32_t BSP_QSPI_EraseBlock(uint32_t Instance, uint32_t BlockAddress, BSP_QSPI_Erase_t BlockSize);
int32_t BSP_QSPI_EraseChip(uint32_t Instance);
int32_t BSP_QSPI_GetStatus(uint32_t Instance);
int32_t BSP_QSPI_GetInfo(uint32_t Instance, BSP_QSPI_Info_t* pInfo);
int32_t BSP_QSPI_EnableMemoryMappedMode(uint32_t Instance);
int32_t BSP_QSPI_DisableMemoryMappedMode(uint32_t Instance);
int32_t BSP_QSPI_ReadID(uint32_t Instance, uint8_t* Id);
int32_t BSP_QSPI_ConfigFlash(uint32_t Instance, BSP_QSPI_Interface_t Mode, BSP_QSPI_Transfer_t Rate);

/* These functions can be modified in case the current settings
   need to be changed for specific application needs */
HAL_StatusTypeDef MX_QSPI_Init(QSPI_HandleTypeDef* hQspi, MX_QSPI_Init_t* Config);

#ifdef __cplusplus
}
#endif

/*********************************************************************************/

void MX_QUADSPI_Init(void);
void MX_QUADSPI_Deinit(void);

uint32_t QSPI_Init(void);
void QSPI_demo(int par, uint32_t addr);
#if 0
void QSPI_ChipErase(void);
void QSPI_MemoryMapped(void);
#endif

void QSPI_ManPage(EResultOut out, int num);

