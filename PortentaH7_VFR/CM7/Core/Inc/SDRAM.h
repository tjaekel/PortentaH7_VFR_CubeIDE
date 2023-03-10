/*
 * SDRAM.h
 *
 *  Created on: Aug 12, 2022
 *      Author: tj
 */

#ifndef INC_SDRAM_H_
#define INC_SDRAM_H_

#define SDRAM_START_ADDRESS		0xC0000000
#define SDRAM_SIZE_BYTES		0x00800000			// 8 MByte

int SDRAM_Init(void);

#define MPU_REGION_SDRAM1	(MPU_REGION_NUMBER4)
#define MPU_REGION_SDRAM2	(MPU_REGION_NUMBER5)

#define MICROPY_HW_SDRAM_TIMING_TMRD        (2)
#define MICROPY_HW_SDRAM_TIMING_TXSR        (7)
#define MICROPY_HW_SDRAM_TIMING_TRAS        (4)
#define MICROPY_HW_SDRAM_TIMING_TRC         (7)
#define MICROPY_HW_SDRAM_TIMING_TWR         (2)
#define MICROPY_HW_SDRAM_TIMING_TRP         (2)
#define MICROPY_HW_SDRAM_TIMING_TRCD        (2)
#define MICROPY_HW_SDRAM_REFRESH_RATE       (64) // ms

#define MICROPY_HW_SDRAM_BURST_LENGTH       2
#define MICROPY_HW_SDRAM_CAS_LATENCY        2
#define MICROPY_HW_SDRAM_COLUMN_BITS_NUM    8
#define MICROPY_HW_SDRAM_ROW_BITS_NUM       12
#define MICROPY_HW_SDRAM_MEM_BUS_WIDTH      16			//why 17
#define MICROPY_HW_SDRAM_INTERN_BANKS_NUM   4
#define MICROPY_HW_SDRAM_CLOCK_PERIOD       2
#define MICROPY_HW_SDRAM_RPIPE_DELAY        1
#define MICROPY_HW_SDRAM_RBURST             (0)
#define MICROPY_HW_SDRAM_WRITE_PROTECTION   (0)
#define MICROPY_HW_SDRAM_AUTOREFRESH_NUM    (4)

#define HW_SDRAM_SIZE						(8 * 1024 * 1024)

#define FMC_SDRAM_BANK						FMC_SDRAM_BANK1
#define FMC_SDRAM_CMD_TARGET_BANK			FMC_SDRAM_CMD_TARGET_BANK1
#define MPU_REGION_SIZE(m)					(((m) - 1) / (((m) - 1) % 255 + 1) / 255 % 255 * 8 + 7 - 86 / (((m) - 1) % 255 + 12) - 1)
#define SDRAM_MPU_REGION_SIZE				(MPU_REGION_SIZE(HW_SDRAM_SIZE))

#define MPU_CONFIG_DISABLE(srd, size) ( \
                                        MPU_INSTRUCTION_ACCESS_DISABLE  << MPU_RASR_XN_Pos \
                                        | MPU_REGION_NO_ACCESS          << MPU_RASR_AP_Pos \
                                        | MPU_TEX_LEVEL0                << MPU_RASR_TEX_Pos \
                                        | MPU_ACCESS_NOT_SHAREABLE      << MPU_RASR_S_Pos \
                                        | MPU_ACCESS_NOT_CACHEABLE      << MPU_RASR_C_Pos \
                                        | MPU_ACCESS_NOT_BUFFERABLE     << MPU_RASR_B_Pos \
                                        | (srd)                         << MPU_RASR_SRD_Pos \
                                        | (size)                        << MPU_RASR_SIZE_Pos \
                                        | MPU_REGION_ENABLE             << MPU_RASR_ENABLE_Pos \
                                      )

#define MPU_CONFIG_SDRAM(size) ( \
                                 MPU_INSTRUCTION_ACCESS_ENABLE   << MPU_RASR_XN_Pos \
                                 | MPU_REGION_FULL_ACCESS        << MPU_RASR_AP_Pos \
                                 | MPU_TEX_LEVEL1                << MPU_RASR_TEX_Pos \
                                 | MPU_ACCESS_NOT_SHAREABLE      << MPU_RASR_S_Pos \
                                 | MPU_ACCESS_CACHEABLE          << MPU_RASR_C_Pos \
                                 | MPU_ACCESS_BUFFERABLE         << MPU_RASR_B_Pos \
                                 | 0x00                          << MPU_RASR_SRD_Pos \
                                 | (size)                        << MPU_RASR_SIZE_Pos \
                                 | MPU_REGION_ENABLE             << MPU_RASR_ENABLE_Pos \
                               )

#endif /* INC_SDRAM_H_ */
