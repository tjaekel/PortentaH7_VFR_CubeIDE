/*
 * QSPI.c
 *
 *  Created on: Apr 15, 2022
 *      Author: Torsten Jaekel
 */

#include "QSPI.h"

QSPI_HandleTypeDef hqspi;

//for STR mode: HALFCYCLE fails!
#define QSPI_SHIFTING_SAMPLES		QSPI_SAMPLE_SHIFTING_NONE
//#define QSPI_SHIFTING_SAMPLES		QSPI_SAMPLE_SHIFTING_HALFCYCLE

/************************ BSP **************************************************/

/**
  * @brief  Return the configuration of the QSPI memory.
  * @param  pInfo pointer on the configuration structure
  * @retval QSPI memory status
  */
int32_t MT25TL01G_GetFlashInfo(MT25TL01G_Info_t* pInfo)
{
    pInfo->FlashSize = MT25TL01G_FLASH_SIZE;
    pInfo->EraseSectorSize = MT25TL01G_SUBSECTOR_SIZE;
    pInfo->ProgPageSize = MT25TL01G_PAGE_SIZE;
    pInfo->EraseSectorsNumber = (MT25TL01G_FLASH_SIZE / pInfo->EraseSectorSize);
    pInfo->ProgPagesNumber = (MT25TL01G_FLASH_SIZE / pInfo->ProgPageSize);
    return MT25TL01G_OK;
}

#if 0
/**
  * @brief  This function set the QSPI memory in 4-byte address mode
  *          SPI/QPI; 1-0-1/4-0-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_Enter4BytesAddressMode(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_ENTER_4_BYTE_ADDR_MODE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /*write enable */
    if (MT25TL01G_WriteEnable(Ctx, Mode) != MT25TL01G_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }
    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Configure automatic polling mode to wait the memory is ready */
    else if (MT25TL01G_AutoPollingMemReady(Ctx, Mode) != MT25TL01G_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }
    return MT25TL01G_OK;
}

/**
  * @brief  Flash exit 4 Byte address mode. Effect 3/4 address byte commands only.
  *         SPI/QPI; 1-0-0/4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_Exit4BytesAddressMode(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_EXIT_4_BYTE_ADDR_MODE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }
    return MT25TL01G_OK;
}
#endif

/**
  * @brief  Polling WIP(Write In Progress) bit become to 0
  *         SPI/QPI;4-0-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_AutoPollingMemReady(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    /* Configure automatic polling mode to wait for memory ready */
    s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES; 	//QSPI_INSTRUCTION_1_LINE; QSPI_INSTRUCTION_4_LINES;
    s_command.Instruction = MT25TL01G_READ_STATUS_REG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_4_LINES;			//QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_8_BITS;
    s_command.Address = 0xC0;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.DummyCycles = 1; //2;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    s_config.Match = 0;
    s_config.MatchMode = QSPI_MATCH_MODE_AND;
    s_config.Interval = 0x10;
    s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;
    s_config.Mask = MT25TL01G_SR_WIP;
    s_config.StatusBytesSize = 1;	//2;

    if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_AUTOPOLLING;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  This function send a Write Enable and wait it is effective.
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_WriteEnable(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef     s_command;
    QSPI_AutoPollingTypeDef s_config;

    /* Enable write operations */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;

    s_command.Instruction = MT25TL01G_WRITE_ENABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Configure automatic polling mode to wait for write enabling */
    s_config.Match = MT25TL01G_SR_WREN;
    s_config.Mask = MT25TL01G_SR_WREN;
    s_config.MatchMode = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval = 0x10;
    s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction = MT25TL01G_READ_STATUS_REG_CMD;
    s_command.DataMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_DATA_4_LINES : QSPI_DATA_1_LINE;

    if (HAL_QSPI_AutoPolling(Ctx, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_AUTOPOLLING;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  This function reset the (WEL) Write Enable Latch bit.
  *         SPI/QPI; 1-0-0/4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_WriteDisable(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef     s_command;
    /* Enable write operations */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_WRITE_DISABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }
    return MT25TL01G_OK;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  *         SPI/QPI; 1-1-1/1-2-2/1-4-4/4-4-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @param  pData Pointer to data to be written
  * @param  WriteAddr Write start address
  * @param  Size Size of data to write. Range 1 ~ 256
  * @retval QSPI memory status
  */
int32_t MT25TL01G_PageProgram(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;

    switch (Mode)
    {

    case MT25TL01G_SPI_MODE:                   /* 1-1-1 commands, Power on H/W default setting */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_PAGE_PROG_CMD;
        s_command.AddressMode = QSPI_ADDRESS_1_LINE;
        s_command.DataMode = QSPI_DATA_1_LINE;
        break;

    case MT25TL01G_SPI_2IO_MODE:               /*  1-2-2 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_EXT_DUAL_IN_FAST_PROG_CMD;
        s_command.AddressMode = QSPI_ADDRESS_2_LINES;
        s_command.DataMode = QSPI_DATA_2_LINES;
        break;

    case MT25TL01G_SPI_4IO_MODE:               /* 1-4-4 program commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_EXT_QUAD_IN_FAST_PROG_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;
        break;

    case MT25TL01G_QPI_MODE:                   /* 4-4-4 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        s_command.Instruction = MT25TL01G_PAGE_PROG_CMD; //MT25TL01G_PAGE_PROG_CMD;	//MT25TL01G_QUAD_IN_FAST_PROG_4_BYTE_ADDR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;
        break;
    }

    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = WriteAddr;
    s_command.DummyCycles = 0;
    s_command.NbData = Size;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;	//QSPI_SIOO_INST_ONLY_FIRST_CMD; QSPI_SIOO_INST_EVERY_CMD;
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }
    if (HAL_QSPI_Transmit(Ctx, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_TRANSMIT;
    }

    //works only this way - or: do we have to poll for 'Write in Progress'?
    osDelay(500);

#if 1
    if (MT25TL01G_AutoPollingMemReady(Ctx, Mode) == MT25TL01G_OK)
    	return MT25TL01G_OK;
    else
    	return MT25TL01G_ERROR_AUTOPOLLING;
#else
    return MT25TL01G_OK;
#endif
}
/**
  * @brief  Reads an amount of data from the QSPI memory on DTR mode.
  *         SPI/QPI; 1-1-1/1-1-2/1-4-4/4-4-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @param  pData Pointer to data to be read
  * @param  ReadAddr Read start address
  * @param  Size Size of data to read
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ReadDTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;
    switch (Mode)
    {
    case MT25TL01G_SPI_MODE:                /* 1-1-1 commands, Power on H/W default setting */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_FAST_READ_4_BYTE_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_1_LINE;
        s_command.DataMode = QSPI_DATA_1_LINE;

        break;
    case MT25TL01G_SPI_2IO_MODE:           /* 1-1-2 read commands */

        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_DUAL_OUT_FAST_READ_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_1_LINE;
        s_command.DataMode = QSPI_DATA_2_LINES;

        break;
    case MT25TL01G_SPI_4IO_MODE:             /* 1-4-4 read commands */

        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;
    case MT25TL01G_QPI_MODE:                 /* 4-4-4 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;
    }
    /* Initialize the read command */
    s_command.DummyCycles = MT25TL01G_DUMMY_CYCLES_READ_QUAD_DTR;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = ReadAddr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.NbData = Size;
    s_command.DdrMode = QSPI_DDR_MODE_ENABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_HALF_CLK_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(Ctx, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_RECEIVE;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Reads an amount of data from the QSPI memory on STR mode.
  *         SPI/QPI; 1-1-1/1-2-2/1-4-4/4-4-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @param  pData Pointer to data to be read
  * @param  ReadAddr Read start address
  * @param  Size Size of data to read
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ReadSTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
    QSPI_CommandTypeDef s_command;
    switch (Mode)
    {
    case MT25TL01G_SPI_MODE:           /* 1-1-1 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_FAST_READ_4_BYTE_ADDR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_1_LINE;
        s_command.DataMode = QSPI_DATA_1_LINE;

        break;
    case MT25TL01G_SPI_2IO_MODE:           /* 1-2-2 read commands */

        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_2_LINES;
        s_command.DataMode = QSPI_DATA_2_LINES;

        break;
    case MT25TL01G_SPI_4IO_MODE:             /* 1-4-4 read commands */

        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;
    case MT25TL01G_QPI_MODE:                 /* 4-4-4 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;
    }

    /* Initialize the read command */
    s_command.DummyCycles = MT25TL01G_DUMMY_CYCLES_READ;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = ReadAddr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.NbData = Size;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(Ctx, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_RECEIVE;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Erases the specified block of the QSPI memory.
  *         MT25TL01G support 4K, 32K, 64K size block erase commands.
  *         SPI/QPI; 1-1-0/4-4-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @param  BlockAddress Block address to erase
  * @retval QSPI memory status
  */
int32_t MT25TL01G_BlockErase(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint32_t BlockAddress, MT25TL01G_Erase_t BlockSize)
{
    QSPI_CommandTypeDef s_command;
    switch (BlockSize)
    {
    default:
    case MT25TL01G_ERASE_4K:
        s_command.Instruction = MT25TL01G_SUBSECTOR_ERASE_CMD_4K;	//MT25TL01G_SUBSECTOR_ERASE_4_BYTE_ADDR_CMD_4K;
        break;

    case MT25TL01G_ERASE_32K:
        s_command.Instruction = MT25TL01G_SUBSECTOR_ERASE_CMD_32K;
        break;

    case MT25TL01G_ERASE_64K:
        s_command.Instruction = MT25TL01G_SECTOR_ERASE_CMD;			//MT25TL01G_SECTOR_ERASE_4_BYTE_ADDR_CMD;
        break;

    case MT25TL01G_ERASE_CHIP:
        return MT25TL01G_ChipErase(Ctx, Mode);
    }
    /* Initialize the erase command */

    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.AddressMode = QSPI_ADDRESS_4_LINES;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = BlockAddress;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    MT25TL01G_WriteEnable(Ctx, Mode);
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }
    if (MT25TL01G_AutoPollingMemReady(Ctx, Mode) == MT25TL01G_OK)
    	return MT25TL01G_OK;
    else
    	return MT25TL01G_ERROR_AUTOPOLLING;
}

/**
  * @brief  Whole chip erase.
  *         SPI/QPI; 1-0-0/4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */

int32_t MT25TL01G_ChipErase(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the erase command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_DIE_ERASE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    if (MT25TL01G_AutoPollingMemReady(Ctx, Mode) == MT25TL01G_OK)
    	return MT25TL01G_OK;
    else
    	return MT25TL01G_ERROR_AUTOPOLLING;

    return MT25TL01G_OK;

}
/**
  * @brief  Read Flash Status register value
  *         SPI/QPI; 1-0-1/4-0-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @param  Value pointer to status register value
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ReadStatusRegister(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* Value)
{
    QSPI_CommandTypeDef s_command;
    /* Initialize the read flag status register command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_READ_STATUS_REG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_DATA_4_LINES : QSPI_DATA_1_LINE;
    s_command.DummyCycles = 0;
    s_command.NbData = 1;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(Ctx, Value, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_RECEIVE;
    }

    return MT25TL01G_OK;
}

int32_t MT25TL01G_ReadConfigurationRegister(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* Value)
{
    QSPI_CommandTypeDef s_command;
    /* Initialize the read flag status register command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_READ_CONFIG_REG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_DATA_4_LINES : QSPI_DATA_1_LINE;
    s_command.DummyCycles = 0;
    s_command.NbData = 1;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(Ctx, Value, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_RECEIVE;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  This function put QSPI memory in QPI mode (Quad I/O) from SPI mode.
  *         SPI -> QPI; 1-x-x -> 4-4-4
  *         SPI; 1-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_EnterQPIMode(QSPI_HandleTypeDef* Ctx)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_ENTER_QUAD_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}
/**
  * @brief  This function put QSPI memory in SPI mode (Single I/O) from QPI mode.
  *         QPI -> SPI; 4-4-4 -> 1-x-x
  *         QPI; 4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ExitQPIMode(QSPI_HandleTypeDef* Ctx)
{
    QSPI_CommandTypeDef s_command;

    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_EXIT_QUAD_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Reads an amount of data from the QSPI memory on DTR mode.
  *         SPI/QPI; 1-1-1/1-1-2/1-4-4/4-4-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_EnableMemoryMappedModeDTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef      s_command;
    QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;
    switch (Mode)
    {
    case MT25TL01G_SPI_MODE:                /* 1-1-1 commands, Power on H/W default setting */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_FAST_READ_4_BYTE_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_1_LINE;
        s_command.DataMode = QSPI_DATA_1_LINE;

        break;
    case MT25TL01G_SPI_2IO_MODE:           /* 1-1-2 read commands */

        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_DUAL_OUT_FAST_READ_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_1_LINE;
        s_command.DataMode = QSPI_DATA_2_LINES;

        break;
    case MT25TL01G_SPI_4IO_MODE:             /* 1-4-4 read commands */

        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;
    case MT25TL01G_QPI_MODE:                 /* 4-4-4 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_DTR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;
    }
    /* Configure the command for the read instruction */
    s_command.AddressSize = QSPI_ADDRESS_32_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = MT25TL01G_DUMMY_CYCLES_READ_QUAD_DTR;
    s_command.DdrMode = QSPI_DDR_MODE_ENABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_HALF_CLK_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the memory mapped mode */
    s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    s_mem_mapped_cfg.TimeOutPeriod = 0;

    if (HAL_QSPI_MemoryMapped(Ctx, &s_command, &s_mem_mapped_cfg) != HAL_OK)
    {
        return MT25TL01G_ERROR_MEMORYMAPPED;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Reads an amount of data from the QSPI memory on STR mode.
  *         SPI/QPI; 1-1-1/1-2-2/1-4-4/4-4-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_EnableMemoryMappedModeSTR(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef      s_command;
    QSPI_MemoryMappedTypeDef s_mem_mapped_cfg;
    switch (Mode)
    {
    case MT25TL01G_SPI_MODE:           		/* 1-1-1 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_FAST_READ_4_BYTE_ADDR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_1_LINE;
        s_command.DataMode = QSPI_DATA_1_LINE;

        break;
    case MT25TL01G_SPI_2IO_MODE:           	/* 1-2-2 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_DUAL_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_2_LINES;
        s_command.DataMode = QSPI_DATA_2_LINES;

        break;

    case MT25TL01G_SPI_4IO_MODE:             /* 1-4-4 read commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_4_BYTE_ADDR_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;

    case MT25TL01G_QPI_MODE:                 /* 4-4-4 commands */
        s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
        s_command.Instruction = MT25TL01G_QUAD_INOUT_FAST_READ_CMD;
        s_command.AddressMode = QSPI_ADDRESS_4_LINES;
        s_command.DataMode = QSPI_DATA_4_LINES;

        break;

    }

    /* Configure the command for the read instruction */
    s_command.DummyCycles = MT25TL01G_DUMMY_CYCLES_READ;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the memory mapped mode */
    s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    s_mem_mapped_cfg.TimeOutPeriod = 0;

    if (HAL_QSPI_MemoryMapped(Ctx, &s_command, &s_mem_mapped_cfg) != HAL_OK)
    {
        return MT25TL01G_ERROR_MEMORYMAPPED;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Flash reset enable command
  *         SPI/QPI; 1-0-0, 4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ResetEnable(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the reset enable command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_RESET_ENABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}


/**
  * @brief  Flash reset memory command
  *         SPI/QPI; 1-0-0, 4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ResetMemory(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the reset enable command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_RESET_MEMORY_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Read Flash 3 Byte IDs.
  *         Manufacturer ID, Memory type, Memory density
  *         SPI/QPI; 1-0-1/4-0-4
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @param  ID pointer to flash id value
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ReadID(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* ID)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the read ID command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = (Mode == MT25TL01G_QPI_MODE) ? MT25TL01G_MULTIPLE_IO_READ_ID_CMD : MT25TL01G_READ_ID_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.DataMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_DATA_4_LINES : QSPI_DATA_1_LINE;
    s_command.NbData = 3;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(Ctx, ID, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_RECEIVE;
    }

    return MT25TL01G_OK;
}

#if 0
/**
  * @brief  Program/Erases suspend. Interruption Program/Erase operations.
  *         After the device has entered Erase-Suspended mode,
  *         system can read any address except the block/sector being Program/Erased.
  *         SPI/QPI; 1-0-0/4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */

int32_t MT25TL01G_ProgEraseSuspend(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the read ID command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_PROG_ERASE_SUSPEND_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Program/Erases resume.
  *         SPI/QPI; 1-0-0/4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ProgEraseResume(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the read ID command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_PROG_ERASE_RESUME_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Deep power down.
  *         The device is not active and all Write/Program/Erase instruction are ignored.
  *         SPI/QPI; 1-0-0/4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_EnterDeepPowerDown(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the read ID command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_ENTER_DEEP_POWER_DOWN;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Release from deep power down.
  *         After CS# go high, system need wait tRES1 time for device ready.
  *         SPI/QPI; 1-0-0/4-0-0
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ReleaseFromDeepPowerDown(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the read ID command */
    s_command.InstructionMode = (Mode == MT25TL01G_QPI_MODE) ? QSPI_INSTRUCTION_4_LINES : QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_RELEASE_FROM_DEEP_POWER_DOWN;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    return MT25TL01G_OK;
}

/**
  * @brief  Read SECTOR PROTECTION Block register value.
  *         SPI; 1-0-1
  * @param  Ctx Component object pointer
  * @param  Mode Interface mode
  * @param  SPBRegister pointer to SPBRegister value
  * @retval QSPI memory status
  */
int32_t MT25TL01G_ReadSPBLockRegister(QSPI_HandleTypeDef* Ctx, MT25TL01G_Interface_t Mode, uint8_t* SPBRegister)
{
    QSPI_CommandTypeDef s_command;

    /* Initialize the reading of SPB lock register command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = MT25TL01G_READ_SECTOR_PROTECTION_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.NbData = 1;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(Ctx, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_COMMAND;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(Ctx, SPBRegister, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return MT25TL01G_ERROR_RECEIVE;
    }

    return MT25TL01G_OK;
}
#endif

/* ============================================================================================ */

BSP_QSPI_Ctx_t     QSPI_Ctx[QSPI_INSTANCES_NUMBER];

static void QSPI_MspInit(QSPI_HandleTypeDef* hQspi);
static void QSPI_MspDeInit(QSPI_HandleTypeDef* hSspi);
static int32_t QSPI_ResetMemory(uint32_t Instance);
#if 0
static int32_t QSPI_DummyCyclesCfg(uint32_t Instance);
#endif

static const uint32_t PrescalerTab[2] = { QSPI_STR_PRESCALER, QSPI_DTR_PRESCALER }; 	//{1, 3};

/**
  * @brief  Initializes the QSPI interface.
  * @param  Instance   QSPI Instance
  * @param  Init       QSPI Init structure
  * @retval BSP status
  */
int32_t BSP_QSPI_Init(uint32_t Instance, BSP_QSPI_Init_t* Init)
{
    int32_t ret = 0;
    BSP_QSPI_Info_t pInfo;
    MX_QSPI_Init_t qspi_init;
    HAL_StatusTypeDef stat;
#if 0
    static uint8_t qspi_aRxBuffer[16];
#endif

    uint8_t id[4];

    /* Table to handle clock prescalers:
    1: For STR mode to reach max 108Mhz
    3: For DTR mode to reach max 54Mhz
    */
    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        ////QSPI_MspDeInit(NULL);
        QSPI_Ctx[Instance].IsInitialized = QSPI_ACCESS_NONE;

        /* Check if instance is already initialized */
        if (QSPI_Ctx[Instance].IsInitialized == QSPI_ACCESS_NONE)
        {
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
            /* Register the QSPI MSP Callbacks */
            if (QSPI_Ctx[Instance].IsMspCallbacksValid == 0UL)
            {
                if (BSP_QSPI_RegisterDefaultMspCallbacks(Instance) != BSP_ERROR_NONE)
                {
                    ret = BSP_ERROR_PERIPH_FAILURE;
                }
            }
#else
            /* Msp QSPI initialization */
            QSPI_MspInit(&hqspi);
#endif /* USE_HAL_QSPI_REGISTER_CALLBACKS */

            if (ret == 0)
            {
                /* STM32 QSPI interface initialization */
                (void)MT25TL01G_GetFlashInfo(&pInfo);
                qspi_init.ClockPrescaler = PrescalerTab[Init->TransferRate];
                qspi_init.DualFlashMode = QSPI_DUALFLASH_DISABLE; /*QSPI_DUALFLASH_ENABLE;*/
                qspi_init.FlashSize = (uint32_t)POSITION_VAL((uint32_t)pInfo.FlashSize) - 1U;
                qspi_init.SampleShifting = (Init->TransferRate == BSP_QSPI_STR_TRANSFER) ? QSPI_SHIFTING_SAMPLES : QSPI_SAMPLE_SHIFTING_NONE;

                stat = MX_QSPI_Init(&hqspi, &qspi_init);
                if (stat != HAL_OK)
                {
                    ret = 2;
                }

                /* QSPI memory reset */
                if (QSPI_ResetMemory(Instance) != 0)
                {
                    ret = 3;
                    print_log(UART_OUT, (const char*)"*E: ResetMemory timeout 1\r\n");
                }

#if 0
                osDelay(200);

                /* Configure Flash to desired mode */
                if (BSP_QSPI_ConfigFlash(Instance, Init->InterfaceMode, Init->TransferRate) != 0)
                {
                    ret = 3;
                }

#else
                else if (MT25TL01G_AutoPollingMemReady(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
                {
                    ret = 3;
                    print_log(UART_OUT, (const char*)"*E: ResetMemory timeout 1\r\n");
                }
                else
                {
#if 0
                    /* Force Flash enter 4 Byte address mode */
                    if (MT25TL01G_Enter4BytesAddressMode(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
                    {
                        ret = 3;
                        print_log(UART_OUT, (const char*)"*E: 4BytesAddress timeout\r\n");
                    }/* Configuration of the dummy cycles on QSPI memory side */
                    else
#endif
#if 0
                    if (QSPI_DummyCyclesCfg(Instance) != 0)
                    {
                        ret = 3;
                    }
                    else
#endif
#if 1
                    {
                        /* Configure Flash to desired mode */
                        if (BSP_QSPI_ConfigFlash(Instance, Init->InterfaceMode, Init->TransferRate) != 0)
                        {
                            ret = 3;
                        }
                    }
#endif
                }
#endif
            }
        }
    }

    id[0] = id[1] = id[2] = 0;
    BSP_QSPI_ReadID(0, id);

    //Portenta H7: should be 0xC22018 = MX25L12833FZ2I-10g
    print_log(UART_OUT, (const char*)"ID: 0x%02x%02x%02x\r\n", id[0], id[1], id[2]);

    return ret;
}

/**
  * @brief  De-Initializes the QSPI interface.
  * @param  Instance   QSPI Instance
  * @retval BSP status
  */
int32_t BSP_QSPI_DeInit(uint32_t Instance)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        if (QSPI_Ctx[Instance].IsInitialized == QSPI_ACCESS_MMP)
        {
            if (BSP_QSPI_DisableMemoryMappedMode(Instance) != 0)
            {
                ret = 3;
            }
        }

        if (ret == 0)
        {
            /* Set default QSPI_Ctx values */
            QSPI_Ctx[Instance].IsInitialized = QSPI_ACCESS_NONE;
            QSPI_Ctx[Instance].InterfaceMode = BSP_QSPI_SPI_MODE;
            QSPI_Ctx[Instance].TransferRate = BSP_QSPI_STR_TRANSFER;
            QSPI_Ctx[Instance].DualFlashMode = QSPI_DUALFLASH_DISABLE;  //QSPI_DUALFLASH_ENABLE;

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 0)
            QSPI_MspDeInit(&hqspi);
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 0) */

            /* Call the DeInit function to reset the driver */
            if (HAL_QSPI_DeInit(&hqspi) != HAL_OK)
            {
                ret = 2;
            }
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief  Initializes the QSPI interface.
  * @param  hQspi       QSPI handle
  * @param  Config      QSPI configuration structure
  * @retval BSP status
  */
/*__weak*/ HAL_StatusTypeDef MX_QSPI_Init(QSPI_HandleTypeDef* hQspi, MX_QSPI_Init_t* Config)
{
    /* QSPI initialization */
    /* QSPI freq = SYSCLK /(1 + ClockPrescaler) Mhz */
    hQspi->Instance = QUADSPI;
    hQspi->Init.ClockPrescaler = Config->ClockPrescaler;
    hQspi->Init.FifoThreshold = 1;
    hQspi->Init.SampleShifting = Config->SampleShifting;
    hQspi->Init.FlashSize = Config->FlashSize;
    hQspi->Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE; /* Min 50ns for nonRead */
    hQspi->Init.ClockMode = QSPI_CLOCK_MODE_0;
    hQspi->Init.FlashID = QSPI_FLASH_ID_1;
    hQspi->Init.DualFlash = Config->DualFlashMode;

    return HAL_QSPI_Init(hQspi);
}

#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP QSPI Msp Callbacks
  * @param Instance      QSPI Instance
  * @retval BSP status
  */
int32_t BSP_QSPI_RegisterDefaultMspCallbacks(uint32_t Instance)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        /* Register MspInit/MspDeInit Callbacks */
        if (HAL_QSPI_RegisterCallback(&hqspi, HAL_QSPI_MSPINIT_CB_ID, QSPI_MspInit) != HAL_OK)
        {
            ret = 2;
        }
        else if (HAL_QSPI_RegisterCallback(&hqspi, HAL_QSPI_MSPDEINIT_CB_ID, QSPI_MspDeInit) != HAL_OK)
        {
            ret = 2;
        }
        else
        {
            QSPI_Ctx[Instance].IsMspCallbacksValid = 1U;
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief BSP QSPI Msp Callback registering
  * @param Instance     QSPI Instance
  * @param CallBacks    pointer to MspInit/MspDeInit callbacks functions
  * @retval BSP status
  */
int32_t BSP_QSPI_RegisterMspCallbacks(uint32_t Instance, BSP_QSPI_Cb_t* CallBacks)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        /* Register MspInit/MspDeInit Callbacks */
        if (HAL_QSPI_RegisterCallback(&hqspi, HAL_QSPI_MSPINIT_CB_ID, CallBacks->pMspInitCb) != HAL_OK)
        {
            ret = 2;
        }
        else if (HAL_QSPI_RegisterCallback(&hqspi, HAL_QSPI_MSPDEINIT_CB_ID, CallBacks->pMspDeInitCb) != HAL_OK)
        {
            ret = 2;
        }
        else
        {
            QSPI_Ctx[Instance].IsMspCallbacksValid = 1U;
        }
    }

    /* Return BSP status */
    return ret;
}
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */

/**
  * @brief  Reads an amount of data from the QSPI memory.
  * @param  Instance  QSPI instance
  * @param  pData     Pointer to data to be read
  * @param  ReadAddr  Read start address
  * @param  Size      Size of data to read
  * @retval BSP status
  */
int32_t BSP_QSPI_Read(uint32_t Instance, uint8_t* pData, uint32_t ReadAddr, uint32_t Size)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        if (QSPI_Ctx[Instance].TransferRate == BSP_QSPI_STR_TRANSFER)
        {
            if (MT25TL01G_ReadSTR(&hqspi, QSPI_Ctx[Instance].InterfaceMode, pData, ReadAddr, Size) != MT25TL01G_OK)
            {
                ret = 2;
            }
        }
        else
        {
            if (MT25TL01G_ReadDTR(&hqspi, QSPI_Ctx[Instance].InterfaceMode, pData, ReadAddr, Size) != MT25TL01G_OK)
            {
                ret = 2;
            }
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief  Writes an amount of data to the QSPI memory.
  * @param  Instance   QSPI instance
  * @param  pData      Pointer to data to be written
  * @param  WriteAddr  Write start address
  * @param  Size       Size of data to write
  * @retval BSP status
  */
int32_t BSP_QSPI_Write(uint32_t Instance, uint8_t* pData, uint32_t WriteAddr, uint32_t Size)
{
#define QSPI_MAX_PRG_SIZE	256

    int ret = 0;
    uint32_t end_addr, current_size, current_addr;
    uint8_t* write_data;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        /* Calculation of the size between the write address and the end of the column */
        current_size = QSPI_MAX_PRG_SIZE - (WriteAddr % QSPI_MAX_PRG_SIZE);

        /* Check if the size of the data is less than the remaining place in the page */
        if (current_size > Size)
        {
            current_size = Size;
        }

        /* Initialize the address variables */
        current_addr = WriteAddr;
        end_addr = WriteAddr + Size;
        write_data = pData;

        /* Perform the write page by page */
        do
        {
        	print_log(UART_OUT, "PR: addr: 0x%06lx | 0x%lx\r\n", current_addr, current_size);

            /* Check if Flash busy ? */
            if (MT25TL01G_AutoPollingMemReady(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
            {
                ret = 5;
            }
            else
            /* Enable write operations */
            if (MT25TL01G_WriteEnable(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
            {
                ret = 4;
            }
            /* Issue page program command */
            else if (MT25TL01G_PageProgram(&hqspi, QSPI_Ctx[Instance].InterfaceMode, write_data, current_addr, current_size) != MT25TL01G_OK)
            {
                ret = 3;
            }
            else
            {
                /* Update the address and size variables for next page programming */
                current_addr += current_size;
                write_data   += current_size;
                current_size  = ((current_addr + QSPI_MAX_PRG_SIZE) > end_addr) ? (end_addr - current_addr) : QSPI_MAX_PRG_SIZE;
            }
        } while ((current_addr < end_addr) && (ret == 0));

        //the very last byte is not written! WHY???
        //even we write the very last again - not working!
    }

    /* Return BSP status */
    if (ret)
    	print_log(UART_OUT, "E* program error: %d\r\n", ret);
    return ret;
}

/**
  * @brief  Erases the specified block of the QSPI memory.
  *         MT25TL01G support 4K, 32K, 64K size block erase commands for each Die.
  *           i.e 8K, 64K, 128K at BSP level (see BSP_QSPI_Erase_t type definition)
  * @param  Instance     QSPI instance
  * @param  BlockAddress Block address to erase
  * @param  BlockSize    Erase Block size
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseBlock(uint32_t Instance, uint32_t BlockAddress, BSP_QSPI_Erase_t BlockSize)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        /* Check Flash busy ? */
        if (MT25TL01G_AutoPollingMemReady(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
        {
            ret = 2;
        }
        else
        /* Enable write operations */
        if (MT25TL01G_WriteEnable(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
        {
            ret = 2;
        }
        else
        {
            /* Issue Block Erase command */
            if (MT25TL01G_BlockErase(&hqspi, QSPI_Ctx[Instance].InterfaceMode, BlockAddress, (MT25TL01G_Erase_t)BlockSize) != MT25TL01G_OK)
            {
                ret = 2;
            }
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief  Erases the entire QSPI memory.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EraseChip(uint32_t Instance)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        /* Check Flash busy ? */
        if (MT25TL01G_AutoPollingMemReady(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
        {
            ret = 2;
        }/* Enable write operations */
        else if (MT25TL01G_WriteEnable(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
        {
            ret = 2;
        }
        else
        {
            /* Issue Chip erase command */
            if (MT25TL01G_ChipErase(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
            {
                ret = 2;
            }
        }
    }

    /* Return BSP status */
    return ret;
}

#if 0
/**
  * @brief  Reads current status of the QSPI memory.
  *         If WIP != 0 then return busy.
  * @param  Instance  QSPI instance
  * @retval QSPI memory status: whether busy or not
  */
int32_t BSP_QSPI_GetStatus(uint32_t Instance)
{
    int32_t ret = 0;
    uint8_t reg;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        if (MT25TL01G_ReadStatusRegister(&hqspi, QSPI_Ctx[Instance].InterfaceMode, &reg) != MT25TL01G_OK)
        {
            ret = 2;
        }
        else
        {
            /* Check the value of the register */
            if ((reg & MT25TL01G_SR_WIP) != 0U)
            {
                ret = 4;
            }
        }
    }

    /* Return BSP status */
    return ret;
}
#endif

int32_t BSP_QSPI_GetConfig(uint32_t Instance, uint8_t *reg)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        if (MT25TL01G_ReadConfigurationRegister(&hqspi, QSPI_Ctx[Instance].InterfaceMode, reg) != MT25TL01G_OK)
        {
            ret = 2;
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief  Return the configuration of the QSPI memory.
  * @param  Instance  QSPI instance
  * @param  pInfo     pointer on the configuration structure
  * @retval BSP status
  */
int32_t BSP_QSPI_GetInfo(uint32_t Instance, BSP_QSPI_Info_t* pInfo)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        (void)MT25TL01G_GetFlashInfo(pInfo);
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief  Configure the QSPI in memory-mapped mode
  *         Only 1 Instance can running MMP mode. And it will lock system at this mode.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_EnableMemoryMappedMode(uint32_t Instance)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        if (QSPI_Ctx[Instance].TransferRate == BSP_QSPI_STR_TRANSFER)
        {
            if (MT25TL01G_EnableMemoryMappedModeSTR(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
            {
                ret =2;
            }
            else /* Update QSPI context if all operations are well done */
            {
                QSPI_Ctx[Instance].IsInitialized = QSPI_ACCESS_MMP;
            }
        }
        else
        {
            if (MT25TL01G_EnableMemoryMappedModeDTR(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
            {
                ret = 2;
            }
            else /* Update QSPI context if all operations are well done */
            {
                QSPI_Ctx[Instance].IsInitialized = QSPI_ACCESS_MMP;
            }
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief  Exit form memory-mapped mode
  *         Only 1 Instance can running MMP mode. And it will lock system at this mode.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
int32_t BSP_QSPI_DisableMemoryMappedMode(uint32_t Instance)
{
    uint8_t Dummy;
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        if (QSPI_Ctx[Instance].IsInitialized != QSPI_ACCESS_MMP)
        {
            ret = 5;
        }/* Abort MMP back to indirect mode */
        else if (HAL_QSPI_Abort(&hqspi) != HAL_OK)
        {
            ret = 1;
        }
        else
        {
            /* Force QSPI interface Sampling Shift to half cycle */
            hqspi.Init.SampleShifting = QSPI_SHIFTING_SAMPLES; //QSPI_SAMPLE_SHIFTING_NONE;	//QSPI_SAMPLE_SHIFTING_HALFCYCLE;

            if (HAL_QSPI_Init(&hqspi) != HAL_OK)
            {
                ret = 1;
            }
            /* Dummy read for exit from Performance Enhance mode */
            else if (MT25TL01G_ReadSTR(&hqspi, QSPI_Ctx[Instance].InterfaceMode, &Dummy, 0, 1) != MT25TL01G_OK)
            {
                ret = 2;
            }
            else /* Update QSPI context if all operations are well done */
            {
                QSPI_Ctx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;
            }
        }
    }
    /* Return BSP status */
    return ret;
}

/**
  * @brief  Get flash ID, 3 Byte
  *         Manufacturer ID, Memory type, Memory density
  * @param  Instance  QSPI instance
  * @param  Id QSPI Identifier
  * @retval BSP status
  */
int32_t BSP_QSPI_ReadID(uint32_t Instance, uint8_t* Id)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        if (MT25TL01G_ReadID(&hqspi, QSPI_Ctx[Instance].InterfaceMode, Id) != MT25TL01G_OK)
        {
            ret = 2;
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief  Set Flash to desired Interface mode. And this instance becomes current instance.
  *         If current instance running at MMP mode then this function isn't work.
  *         Indirect -> Indirect
  * @param  Instance  QSPI instance
  * @param  Mode      QSPI mode
  * @param  Rate      QSPI transfer rate
  * @retval BSP status
  */
int32_t BSP_QSPI_ConfigFlash(uint32_t Instance, BSP_QSPI_Interface_t Mode, BSP_QSPI_Transfer_t Rate)
{
    int32_t ret = 0;

    /* Check if the instance is supported */
    if (Instance >= QSPI_INSTANCES_NUMBER)
    {
        ret = 1;
    }
    else
    {
        /* Check if MMP mode locked ************************************************/
        if (QSPI_Ctx[Instance].IsInitialized == QSPI_ACCESS_MMP)
        {
            ret = 5;
        }
        else
        {
            /* Setup MCU transfer rate setting ***************************************************/
            hqspi.Init.SampleShifting = (Rate == BSP_QSPI_STR_TRANSFER) ? QSPI_SHIFTING_SAMPLES : QSPI_SAMPLE_SHIFTING_NONE; //QSPI_SAMPLE_SHIFTING_HALFCYCLE

            if (HAL_QSPI_Init(&hqspi) != HAL_OK)
            {
                ret = 1;

            }
            else
            {
                /* Setup Flash interface ***************************************************/
                switch (QSPI_Ctx[Instance].InterfaceMode)
                {
                case MT25TL01G_QPI_MODE:               /* 4-4-4 commands */
                    if (Mode != MT25TL01G_QPI_MODE)
                    {
                        if (MT25TL01G_ExitQPIMode(&hqspi) != MT25TL01G_OK)
                        {
                            ret = 2;
                        }
                    }
                    break;

                case BSP_QSPI_SPI_MODE:               	/* 1-1-1 commands, Power on H/W default setting */
                case BSP_QSPI_SPI_2IO_MODE:           	/* 1-2-2 read commands */
                case BSP_QSPI_SPI_4IO_MODE:           	/* 1-4-4 read commands */
                /* fall through! */
                default:
                    if (Mode == MT25TL01G_QPI_MODE)
                    {
                        if (MT25TL01G_EnterQPIMode(&hqspi) != MT25TL01G_OK)
                        {
                            ret = 2;
                        }
                    }
                    break;
                }

                /* Update QSPI context if all operations are well done */
                if (ret == 0)
                {
                    /* Update current status parameter *****************************************/
                    QSPI_Ctx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;
                    QSPI_Ctx[Instance].InterfaceMode = Mode;
                    QSPI_Ctx[Instance].TransferRate  = Rate;
                }
            }
        }
    }

    /* Return BSP status */
    return ret;
}

/**
  * @brief QSPI MSP Initialization
  * @param hQspi : QSPI handle
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - NVIC configuration for QSPI interrupt
  * @retval None
  */
static void QSPI_MspInit(QSPI_HandleTypeDef* hQspi)
{
#if 0
    GPIO_InitTypeDef gpio_init_structure;
#endif

    /* Prevent unused argument(s) compilation warning */
    UNUSED(hQspi);

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable the QuadSPI memory interface clock */
    QSPI_CLK_ENABLE();
    /* Reset the QuadSPI memory interface */
    QSPI_FORCE_RESET();
    QSPI_RELEASE_RESET();
}

/**
  * @brief QSPI MSP De-Initialization
  * @param hQspi  QSPI handle
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO and NVIC configuration to their default state
  * @retval None
  */
static void QSPI_MspDeInit(QSPI_HandleTypeDef* hQspi)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hQspi);

    /*##-3- Reset peripherals ##################################################*/
    /* Reset the QuadSPI memory interface */
    QSPI_FORCE_RESET();
    QSPI_RELEASE_RESET();

    /* Disable the QuadSPI memory interface clock */
    QSPI_CLK_DISABLE();
}

/**
  * @brief  This function reset the QSPI Flash memory.
  *         Fore QPI+SPI reset to avoid system come from unknown status.
  *         Flash accept 1-1-1, 1-1-2, 1-2-2 commands after reset.
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
static int32_t QSPI_ResetMemory(uint32_t Instance)
{
    int32_t ret = 0;

    /* Send RESET ENABLE command in QPI mode (QUAD I/Os, 4-4-4) */
    if (MT25TL01G_ResetEnable(&hqspi, MT25TL01G_QPI_MODE) != MT25TL01G_OK)
    {
        ret = 2;
    }
    else
    /* Send RESET memory command in QPI mode (QUAD I/Os, 4-4-4) */
    if (MT25TL01G_ResetMemory(&hqspi, MT25TL01G_QPI_MODE) != MT25TL01G_OK)
    {
        ret = 2;
    }
    /* Wait Flash ready */
    else if (MT25TL01G_AutoPollingMemReady(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
    {
        ret = 2;
    }
    
    else
    /* Send RESET ENABLE command in SPI mode (1-1-1) */
    if (MT25TL01G_ResetEnable(&hqspi, BSP_QSPI_SPI_MODE) != MT25TL01G_OK)
    {
        ret = 2;
    }
    else
    /* Send RESET memory command in SPI mode (1-1-1) */
    if (MT25TL01G_ResetMemory(&hqspi, BSP_QSPI_SPI_MODE) != MT25TL01G_OK)
    {
        ret = 2;
    }
    else
    {
        QSPI_Ctx[Instance].IsInitialized = QSPI_ACCESS_INDIRECT;  /* After reset S/W setting to indirect access   */
        QSPI_Ctx[Instance].InterfaceMode = BSP_QSPI_SPI_MODE;     /* After reset H/W back to SPI mode by default  */
        QSPI_Ctx[Instance].TransferRate  = BSP_QSPI_STR_TRANSFER; /* After reset S/W setting to STR mode          */
    }

    /* Return BSP status */
    return ret;
}

#if 0
/**
  * @brief  This function configure the dummy cycles on memory side.
  *         Dummy cycle bit locate in Configuration Register[7:6]
  * @param  Instance  QSPI instance
  * @retval BSP status
  */
static int32_t QSPI_DummyCyclesCfg(uint32_t Instance)
{
    int32_t ret = 0;
    QSPI_CommandTypeDef s_command;
    uint16_t reg = 0;

    /* Initialize the read volatile configuration register command */
    s_command.InstructionMode = QSPI_INSTRUCTION_4_LINES;
    s_command.Instruction = MT25TL01G_READ_VOL_CFG_REG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_4_LINES;
    s_command.DummyCycles = 0;
    s_command.NbData = 2;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 3;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(&hqspi, (uint8_t*)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 3;
    }

    /* Enable write operations */
    if (MT25TL01G_WriteEnable(&hqspi, QSPI_Ctx[Instance].InterfaceMode) != MT25TL01G_OK)
    {
        return 3;
    }

    /* Update volatile configuration register (with new dummy cycles) */
    s_command.Instruction = MT25TL01G_WRITE_VOL_CFG_REG_CMD;
    MODIFY_REG(reg, 0xF0F0, ((MT25TL01G_DUMMY_CYCLES_READ_QUAD << 4) |
        (MT25TL01G_DUMMY_CYCLES_READ_QUAD << 12)));

    /* Configure the write volatile configuration register command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 3;
    }

    /* Transmission of the data */
    if (HAL_QSPI_Transmit(&hqspi, (uint8_t*)(&reg), HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 3;
    }

    /* Return BSP status */
    return ret;
}
#endif

/*******************************************************************************/

/**
* @brief QSPI MSP Initialization
* This function configures the hardware resources used in this example
* @param hqspi: QSPI handle pointer
* @retval None
*/
void HAL_QSPI_MspInit(QSPI_HandleTypeDef* hqspi)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    if (hqspi->Instance == QUADSPI)
    {
#if 0
        /** Initializes the peripherals clock */
    	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    	/* ATT: it modifies the PLL N, R and Q registers!! potentially their values have to be provided here! */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_QSPI;
        PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_PLL2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            ////Error_Handler();
        }
#endif

        /* Peripheral clock enable - done already */
        ////__HAL_RCC_QSPI_CLK_ENABLE();

        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        /**QUADSPI GPIO Configuration
        PF10     ------> QUADSPI_CLK
        PD11     ------> QUADSPI_BK1_IO0
        PD12     ------> QUADSPI_BK1_IO1
        PF7      ------> QUADSPI_BK1_IO2
        PD13     ------> QUADSPI_BK1_IO3
        PG6      ------> QUADSPI_BK1_NCS
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        /* USER CODE BEGIN QUADSPI_MspInit 1 */
          /* NVIC configuration for QSPI interrupt */
        HAL_NVIC_SetPriority(QUADSPI_IRQn, 0x0F, 0);
        HAL_NVIC_EnableIRQ(QUADSPI_IRQn);
    }
}

/**
* @brief QSPI MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hqspi: QSPI handle pointer
* @retval None
*/
void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef* hqspi)
{
    (void)hqspi;

    ////if(hqspi->Instance==QUADSPI)
    {
        HAL_NVIC_DisableIRQ(QUADSPI_IRQn);

        /* USER CODE END QUADSPI_MspDeInit 0 */
          /* Peripheral clock disable */
        __HAL_RCC_QSPI_CLK_DISABLE();

        /**QUADSPI GPIO Configuration
        PF10     ------> QUADSPI_CLK
        PD11     ------> QUADSPI_BK1_IO0
        PD12     ------> QUADSPI_BK1_IO1
        PF7      ------> QUADSPI_BK1_IO2
        PD13     ------> QUADSPI_BK1_IO3
        PG6      ------> QUADSPI_BK1_NCS
        */
        HAL_GPIO_DeInit(GPIOG, GPIO_PIN_6);

        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);

        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_7 | GPIO_PIN_10);
    }
}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
void MX_QUADSPI_Init(void)
{
    /* USER CODE BEGIN QUADSPI_Init 0 */

    /* USER CODE END QUADSPI_Init 0 */

    /* USER CODE BEGIN QUADSPI_Init 1 */

    /* USER CODE END QUADSPI_Init 1 */
    /* QUADSPI parameter configuration*/
    hqspi.Instance = QUADSPI;
    hqspi.Init.ClockPrescaler = QSPI_START_PRESCALER;
    hqspi.Init.FifoThreshold = 1;
    hqspi.Init.SampleShifting = QSPI_SHIFTING_SAMPLES;	//QSPI_SAMPLE_SHIFTING_HALFCYCLE;	//QSPI_SAMPLE_SHIFTING_NONE;
    hqspi.Init.FlashSize = 1;
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;	//QSPI_CS_HIGH_TIME_1_CYCLE;
    hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&hqspi) != HAL_OK)
    {
        ////Error_Handler();
    }
}

void MX_QUADSPI_Deinit(void)
{
    HAL_QSPI_MspDeInit(&hqspi);
}

/* ============================================================================================== */

#define BUFFER_SIZE         ((uint32_t)0x200)			//4K = 0x1000 is max. for page program!

uint8_t* qspi_aTxBuffer;
uint8_t* qspi_aRxBuffer;

static void     Fill_Buffer(uint8_t* pBuffer, uint32_t uwBufferLength, uint32_t uwOffset);
static uint8_t  Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint32_t BufferLength);

#if 0
void QSPI_ChipErase(void)
{
    //NOT SURE: disable memory mapped
    BSP_QSPI_DisableMemoryMappedMode(0);

    if (BSP_QSPI_EraseChip(0) != 0)
        //if(BSP_QSPI_EraseBlock(0, WRITE_READ_ADDR, BSP_QSPI_ERASE_8K) != BSP_ERROR_NONE)
    {
        print_log(UART_OUT, (const char*)"*E: QSPI erase failed\n\r");
    }
    else
    {
        print_log(UART_OUT, (const char*)"*I: QSPI erase OK\n\r");
    }

    //clear the cache
    ////SCB_CleanDCache_by_Addr((uint32_t*)0x90000000, 1024 * 1024);
    ////BSP_QSPI_EnableMemoryMappedMode(0);
}
#endif

uint32_t QSPI_Init()
{
    static BSP_QSPI_Init_t init;
    init.InterfaceMode = MT25TL01G_QPI_MODE;			//MT25TL01G_SPI_MODE; MT25TL01G_SPI_4IO_MODE; MT25TL01G_QPI_MODE;
    init.TransferRate  = MT25TL01G_STR_TRANSFER;  	    //MT25TL01G_STR_TRANSFER; MT25TL01G_DTR_TRANSFER;
    init.DualFlashMode = MT25TL01G_DUALFLASH_DISABLE;   //MT25TL01G_DUALFLASH_ENABLE;

    return BSP_QSPI_Init(0, &init);
}

/**
  * @brief  QSPI Demo
  * @param  par number 0,1 (read), 2 (write) 3 (erase all)
  * @retval None
  */
void QSPI_demo(int par, uint32_t addr)
{
    qspi_aTxBuffer = MEM_PoolAlloc(MEM_POOL_SEG_SIZE);
    if (!qspi_aTxBuffer)
        return;
    qspi_aRxBuffer = MEM_PoolAlloc(MEM_POOL_SEG_SIZE);
    if (!qspi_aRxBuffer)
    {
        MEM_PoolFree(qspi_aTxBuffer);
        return;
    }

    /* QSPI info structure */
    BSP_QSPI_Info_t pQSPI_Info;
    uint8_t status = 0;

#if 0
    /* QSPI device configuration */
    BSP_QSPI_Init_t init;
    init.InterfaceMode = MT25TL01G_QPI_MODE;			//MT25TL01G_SPI_MODE; MT25TL01G_SPI_4IO_MODE; MT25TL01G_QPI_MODE;
    init.TransferRate  = MT25TL01G_STR_TRANSFER;  	    //MT25TL01G_STR_TRANSFER; MT25TL01G_DTR_TRANSFER;
    init.DualFlashMode = MT25TL01G_DUALFLASH_DISABLE;   //MT25TL01G_DUALFLASH_ENABLE;
#endif

    /* convert addr as index for 4K page to real addr */
    addr = addr * MT25TL01G_SUBSECTOR_SIZE;

    if (par == 0)
    {
#if 0
    	BSP_QSPI_DeInit(0);
#if 0
    	status = BSP_QSPI_Init(0, &init);
#else
    	status = QSPI_Init();
#endif
#endif

    	if (status != 0)
    	{
    		print_log(UART_OUT, (const char*)"*E: QSPI error: %x\n\r", status);
    	}
    	else
    	{
    		print_log(UART_OUT, (const char*)"*I: QSPI OK\n\r");

    		/* Initialize the structure */
    		pQSPI_Info.FlashSize = (uint32_t)0x00;
    		pQSPI_Info.EraseSectorSize = (uint32_t)0x00;
    		pQSPI_Info.EraseSectorsNumber = (uint32_t)0x00;
    		pQSPI_Info.ProgPageSize = (uint32_t)0x00;
    		pQSPI_Info.ProgPagesNumber = (uint32_t)0x00;

    		/* Read the QSPI memory info */
    		BSP_QSPI_GetInfo(0, &pQSPI_Info);
    		{
    			uint8_t id[3];
    			id[0] = id[1] = id[2] = 0;
    			BSP_QSPI_ReadID(0, id);
    			//Portenta H7: should be 0xC22018
    			print_log(UART_OUT, (const char*)"ID: 0x%02x%02x%02x\r\n", id[0], id[1], id[2]);
    		}

    		/* Test the correctness */
    		if ((pQSPI_Info.FlashSize != 0x1000000) || (pQSPI_Info.EraseSectorSize != 0x1000) ||
    				(pQSPI_Info.ProgPageSize != 0x100) || (pQSPI_Info.EraseSectorsNumber != 0x1000) ||
					(pQSPI_Info.ProgPagesNumber != 0x10000))
    		{
    			print_log(UART_OUT, (const char*)"*E: QSPI info failed\r\n");
    			print_log(UART_OUT, (const char*)"    %lx | %lx | %lx | %lx | %lx\r\n",
    					pQSPI_Info.FlashSize,
						pQSPI_Info.EraseSectorSize,
						pQSPI_Info.ProgPageSize,
						pQSPI_Info.EraseSectorsNumber,
						pQSPI_Info.ProgPagesNumber
    			);
    		}
    		else
    		{
    			print_log(UART_OUT, (const char*)"*I: QSPI info OK\n\r");
    		}

    		{
    			uint8_t cfg;
    			////int32_t status;
    			/*status = */ BSP_QSPI_GetConfig(0, &cfg);
    			print_log(UART_OUT, (const char*)"*I: config: %02x\r\n", cfg);
    		}
    	}
    }

    if (par == 3)
    {
                if(BSP_QSPI_EraseBlock(0, addr, BSP_QSPI_ERASE_4K) != 0)
                {
                    print_log(UART_OUT, (const char*)"*E: QSPI erase failed\n\r");
                }
                else
                {
                    print_log(UART_OUT, (const char*)"*I: QSPI erase OK\n\r");
                }
    }
    if (par == 2)
    {
                /* Fill the buffer to write */
                Fill_Buffer(&qspi_aTxBuffer[0], BUFFER_SIZE, ((addr >> 8) & 0xFF) + ((addr >> 12) & 0xFF));

                /* Write data to the QSPI memory */
                if (BSP_QSPI_Write(0, qspi_aTxBuffer, addr, BUFFER_SIZE) != 0)
                {
                    print_log(UART_OUT, (const char*)"*E: QSPI write failed\n\r");
                }
                else
                {
                    print_log(UART_OUT, (const char*)"*I: QSPI write OK\n\r");
                }
    }
    if (par == 1)
    {
                    /* Read back data from the QSPI memory */
                    if (BSP_QSPI_Read(0, qspi_aRxBuffer, addr, BUFFER_SIZE) != 0)
                    {
                        print_log(UART_OUT, (const char*)"*E: QSPI read failed\n\r");
                    }
                    else
                    {
                    	Fill_Buffer(&qspi_aTxBuffer[0], BUFFER_SIZE, ((addr >> 8) & 0xFF) + ((addr >> 12) & 0xFF));

                        print_log(UART_OUT, (const char*)"*I: QSPI read OK\n\r");
                        hex_dump((uint8_t *)qspi_aRxBuffer, BUFFER_SIZE, 1, UART_OUT);

                        /*##-5- Checking data integrity ############################################*/
                        if (Buffercmp(qspi_aRxBuffer, &qspi_aTxBuffer[0], BUFFER_SIZE) > 0)
                        {
                            print_log(UART_OUT, (const char*)"*E: QSPI compare failed\n\r");
                        }
                    }
    }
    if (par == 4)
    {
                    if (BSP_QSPI_EnableMemoryMappedMode(0) != 0)
                    {
                        print_log(UART_OUT, (const char*)"*E: QSPI memory mapped failed\n\r");
                    }
                    else
                    {
                        ////print_log(UART_OUT, (const char*)"*I: QSPI memory mapped OK\n\r");
                    }
    }
    if (par == 5)
    {
    	if (BSP_QSPI_DisableMemoryMappedMode(0) != 0)
    	{
    	     print_log(UART_OUT, (const char*)"*E: QSPI memory mapped disable failed\n\r");
    	}
    }
    if (par == 6)
    {
    	BSP_QSPI_EraseChip(0);
    }

    MEM_PoolFree(qspi_aTxBuffer);
    MEM_PoolFree(qspi_aRxBuffer);
}

/**
  * @brief  Fills buffer with user predefined data.
  * @param  pBuffer: pointer on the buffer to fill
  * @param  uwBufferLenght: size of the buffer to fill
  * @param  uwOffset: first value to fill on the buffer
  * @retval None
  */
static void Fill_Buffer(uint8_t* pBuffer, uint32_t uwBufferLength, uint32_t uwOffset)
{
    uint32_t tmpIndex;

    /* Put in global buffer different values */
    for (tmpIndex = 0; tmpIndex < uwBufferLength; tmpIndex++)
    {
        pBuffer[tmpIndex] = tmpIndex + uwOffset;
    }
}

/**
  * @brief  Compares two buffers.
  * @param  pBuffer1, pBuffer2: buffers to be compared.
  * @param  BufferLength: buffer's length
  * @retval 1: pBuffer identical to pBuffer1
  *         0: pBuffer differs from pBuffer1
  */
static uint8_t Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint32_t BufferLength)
{
    uint8_t err = 0;
    int i = 0;
    int j = 0;
    while (BufferLength--)
    {
        ////print_log(UART_OUT, (const char*)"*I: %ld | %02x %02x\r\n", BufferLength, *pBuffer1, *pBuffer2);
        if (*pBuffer1 != *pBuffer2)
        {
            print_log(UART_OUT, (const char*)"*E: %d | %02x != %02x\r\n", j, *pBuffer1, *pBuffer2);
            err = 1;
            i++;
        }

        if (i > 10)
            break;

        pBuffer1++;
        pBuffer2++;
        j++;
    }

    return err;
}

#if 0
void QSPI_MemoryMapped(void) {
    BSP_QSPI_Init_t init;
    init.InterfaceMode = MT25TL01G_QPI_MODE;			//MT25TL01G_SPI_4IO_MODE; MT25TL01G_QPI_MODE;
    init.TransferRate = MT25TL01G_STR_TRANSFER;  	    //MT25TL01G_STR_TRANSFER; MT25TL01G_DTR_TRANSFER;
    init.DualFlashMode = MT25TL01G_DUALFLASH_DISABLE;   //MT25TL01G_DUALFLASH_ENABLE;
    BSP_QSPI_Init(0, &init);

    /* the first time it fails - WHY? chip ID is wrong, do it a second time: OK */
    BSP_QSPI_Init(0, &init);

    /* it works just a little bit! the chip on Portenta board is not really nice (too old),
    * it does not support really the instruction needed for memory mapped access (it needs two instructions
    * to do a read!)
    */
    BSP_QSPI_EnableMemoryMappedMode(0);
}
#endif

void QSPI_ManPage(EResultOut out, int num)
{
	/* display a man page: assume QSPI is initialized and memory mapped
	 * check if programmed (not 0xFF) and print all (from 4K sector) until 0xFF or 0x00
	 */

	/* TODO: check if initialized and memory mapped */

    uint8_t *ptr = (uint8_t *)QSPI_BASE_ADDRESS;

    ptr += num * MT25TL01G_SUBSECTOR_SIZE;
    while ((*ptr != 0xFF) && (*ptr != 0x00))
    {
    	/* just on UART - we could verify max. length, assume 4K sector is finished */
    	UART_Send(ptr++, 1, out);
    }
}
