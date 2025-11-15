#pragma once
/*
 * mx25v1635f.h
 *
 *  Created on: Nov 1, 2024
 *      Author: Belina Sainju
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// Flash ID
#define FLASH_DEVICE_ID 0xC22315 // Manufacturer ID (C2h), Memory Type (23h), Device ID (15h)
#define ELECTRONIC_ID 0x15
#define REMS_ID_0 0xC215
#define REMS_ID_1 0x15C2
#define FLASH_SIZE 0x200000 // 2MB

// Timing values taken from datasheet
#define tPP 4     // 4ms
#define tSE 240   // Sector Erase Cycle time max 240ms
#define tPUW 10   // 10 ms (Value taken from Macronix LLD reference)
#define tDP 1     // 10us (Transition time from StandBy mode to DeepPowerDown mode)
#define tDPDD 1   // 30us (Delay time to release from deep power down mode)
#define tCRDP 1   // 20ns (Min time CS needs to be low to wake flash)
#define tRDP 1    // 45us (Transition time from DeepPowerDown mode to StandBy mode)
#define tCE 38000 // 38sec

#define PAGE_PROGRAM_CYCLE_TIME tPP
#define SECTOR_ERASE_CYCLE_TIME tSE
#define FLASH_FULL_ACCESS_TIME tPUW
#define STANDBY_TO_DP_MODE_DELAY tDP
#define WAKE_UP_CS_PIN_LOW_TIME tCRDP
#define DP_TO_STANDBY_MODE_DELAY tRDP
#define CHIP_ERASE_CYCLE_TIME tCE

/*
  Flash Related Parameter Define
*/

#define BLOCK_OFFSET 0x10000    // 64K Block size
#define BLOCK_32K_OFFSET 0x8000 // 32K Block size
#define SECTOR_OFFSET 0x1000    // 4K Sector size
#define PAGE_OFFSET 0x0100      // 256 Byte Page size
#define PAGE_32_OFFSET 0x0020   // 32 Byte Page size (some products have smaller page size)
#define BLOCK_NUM (FLASH_SIZE / BLOCK_OFFSET)

// Flash control register mask define
// status register
#define FLASH_WIP_MASK 0x01
#define FLASH_LDSO_MASK 0x02
#define FLASH_QE_MASK 0x40
// security register
#define FLASH_OTPLOCK_MASK 0x03
#define FLASH_4BYTE_MASK 0x04
#define FLASH_WPSEL_MASK 0x80
// configuration reigster
#define FLASH_DC_MASK 0x80
#define FLASH_DC_2BIT_MASK 0xC0
#define FLASH_DC_3BIT_MASK 0x07
// other
#define BLOCK_PROTECT_MASK 0xff
#define BLOCK_LOCK_MASK 0x01

/*
    Flash Commands
*/
// ID Commands
#define FLASH_CMD_RDID 0x9F // RDID (Read Identification)
#define FLASH_CMD_RES 0xAB  // RES (Read Electronic ID)
#define FLASH_CMD_REMS 0x90 // REMS (Read Electronic & Device ID)

// Register comands
#define FLASH_CMD_WRSR 0x01   // WRSR (Write Status Register)
#define FLASH_CMD_RDSR 0x05   // RDSR (Read Status Register)
#define FLASH_CMD_WRSCUR 0x2F // WRSCUR (Write Security Register)
#define FLASH_CMD_RDSCUR 0x2B // RDSCUR (Read Security Register)
#define FLASH_CMD_RDCR 0x15   // RDCR (Read Configuration Register)

// READ comands
#define FLASH_CMD_READ 0x03     // READ (1 x I/O)
#define FLASH_CMD_2READ 0xBB    // 2READ (2 x I/O)
#define FLASH_CMD_4READ 0xEB    // 4READ (4 x I/O)
#define FLASH_CMD_FASTREAD 0x0B // FAST READ (Fast read data)
#define FLASH_CMD_DREAD 0x3B    // DREAD (1In/2 Out fast read)
#define FLASH_CMD_QREAD 0x6B    // QREAD (1In/4 Out fast read)
#define FLASH_CMD_RDSFDP 0x5A   // RDSFDP (Read SFDP)

// Program comands
#define FLASH_CMD_WREN 0x06 // WREN (Write Enable)
#define FLASH_CMD_WRDI 0x04 // WRDI (Write Disable)
#define FLASH_CMD_PP 0x02   // PP (page program)
#define FLASH_CMD_4PP 0x38  // 4PP (Quad page program)

// Erase comands
#define FLASH_CMD_SE 0x20    // SE (Sector Erase)
#define FLASH_CMD_BE32K 0x52 // BE32K (Block Erase 32kb)
#define FLASH_CMD_BE 0xD8    // BE (Block Erase)
#define FLASH_CMD_CE 0x60    // CE (Chip Erase) hex code: 60 or C7

// Mode setting comands
#define FLASH_CMD_DP 0xB9   // DP (Deep Power Down)
#define FLASH_CMD_ENSO 0xB1 // ENSO (Enter Secured OTP)
#define FLASH_CMD_EXSO 0xC1 // EXSO  (Exit Secured OTP)
#ifdef SBL_CMD_0x77
#define FLASH_CMD_SBL 0x77 // SBL (Set Burst Length) new: 0x77
#else
#define FLASH_CMD_SBL 0xC0 // SBL (Set Burst Length) Old: 0xC0
#endif

// Reset comands
#define FLASH_CMD_RSTEN 0x66 // RSTEN (Reset Enable)
#define FLASH_CMD_RST 0x99   // RST (Reset Memory)

// Security comands
#ifdef LCR_CMD_0xDD_0xD5
#else
#endif

// Suspend/Resume comands
#define FLASH_CMD_PGM_ERS_S 0xB0 // PGM/ERS Suspend (Suspends Program/Erase)
#define FLASH_CMD_PGM_ERS_R 0x30 // PGM/ERS Erase (Resumes Program/Erase)
#define FLASH_CMD_NOP 0x00       // NOP (No Operation)

// Return Message
typedef enum
{
    FLASH_OPERATION_SUCCESS,
    FLASH_OPERATION_FAILED,
    FLASH_WRITE_REG_FAILED,
    FLASH_TIME_OUT,
    FLASH_IS_BUSY,
    FLASH_QUAD_NOT_ENABLE,
    FLASH_ADDRESS_INVALID
} flashReturnMsg_t;

// Flash status structure define
typedef struct
{
    /* Mode Register:
     * Bit  Description
     * -------------------------
     *  7   RYBY enable
     *  6   Reserved
     *  5   Reserved
     *  4   Reserved
     *  3   Reserved
     *  2   Reserved
     *  1   Parallel mode enable
     *  0   QPI mode enable
     */
    uint8_t ModeReg;
    bool ArrangeOpt;
} FlashStatus_t;

// =============================================================================================#=
// Public API Functions
// =============================================================================================#=

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// One-time startup initialization for the MX25V1635F Flash
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
bool MX25_Init(void);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Functions for ID commands
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
flashReturnMsg_t MX25_RDID(uint32_t *identification);
flashReturnMsg_t MX25_RES(uint8_t *electronicIdentification);
flashReturnMsg_t MX25_REMS(uint16_t *remsIdentification, FlashStatus_t *fsptr);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Functions for register setting commands
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
flashReturnMsg_t MX25_RDSR(uint8_t *statusReg);
flashReturnMsg_t MX25_RDSCUR(uint8_t *securityReg);
flashReturnMsg_t MX25_WREN(void);
flashReturnMsg_t MX25_DP(void);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Functions for read/write array commands
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
flashReturnMsg_t MX25_READ(uint32_t flashAddress, uint8_t *targetAddress, uint32_t byteLength);
flashReturnMsg_t MX25_PP(uint32_t flashAddress, uint8_t *sourceAddress, uint32_t byteLength);
flashReturnMsg_t MX25_SE(uint32_t flashAddress);
flashReturnMsg_t MX25_CE(void);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Other Public API functions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void MX25_WAKE(void);
