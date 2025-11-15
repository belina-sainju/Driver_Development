/*
 * mx25v1635f.c
 *
 *  Created on: Nov 1, 2024
 *      Author: Belina Sainju
 */

#include "mx25v1635f.h"

#include "board-model.h"
#include "../platform/gpio/gpio.h"
#include "../platform/spi/spi-core.h"

#include "stm32f4xx_hal_gpio.h"

#include "FreeRTOS.h"
#include "task.h"

/*** Private  Functions ***/

// Helper function to set CS pin high
static void flashChipSelectHigh(void)
{
    HAL_GPIO_WritePin(PORT(FLASH_CS), PIN(FLASH_CS), GPIO_PIN_SET);
}

// Helper function to set CS pin low
static void flashChipSelectLow(void)
{
    HAL_GPIO_WritePin(PORT(FLASH_CS), PIN(FLASH_CS), GPIO_PIN_RESET);
}

/*
 * Function:       flashInsertDummyCycle
 * Arguments:      dummyCycle, number of dummy clock cycle
 * Description:    Insert dummy cycle of SCLK
 * Return Message: None.
 */
static bool flashInsertDummyCycle(uint8_t dummyCycle)
{
    uint8_t dummyClockCycleValue = 0xFF;
    bool status = true;

    for (uint8_t i = 0; i < dummyCycle; i++)
    {
        if (status)
        {
            status = SPI_Transfer(MX25_FLASH, &dummyClockCycleValue, sizeof(dummyClockCycleValue), NULL, 0);
        }
        else
        {
            return status;
        }
    }
    return status;
}

/*
 * Function:       flashIs4Byte
 * Arguments:      None
 * Description:    Check flash address is 3-byte or 4-byte.
 *                 If flash 4BYTE bit = 1: return true
 *                                    = 0: return false.
 * Return Message: true, false
 */
static bool flashIs4Byte(void)
{
#ifdef FLASH_CMD_RDSCUR
#ifdef FLASH_4BYTE_ONLY
    return true;
#elif FLASH_3BYTE_ONLY
    return false;
#else
    uint8_t dataBuffer;
    MX25_RDSCUR(&dataBuffer);
    if ((dataBuffer & FLASH_4BYTE_MASK) == FLASH_4BYTE_MASK)
        return true;
    else
        return false;
#endif
#else
    return false;
#endif
}

/*
 * Function:       Read FLASH Identification
 * Arguments:      regToRead, commandLength, dataReceived, lengthToReceive
 * Description:    Sends FLASH command and stores data read in dataReceived
 * Return Message: Bool
 */
static bool flashRead(uint8_t *command, uint8_t commandLength, uint8_t *dataReceived, uint8_t lengthToReceive)
{
    bool status = false;
    uint8_t readCommand = (uint8_t)*command;

    flashChipSelectLow();
    status = SPI_Transfer(MX25_FLASH, (uint8_t *)&readCommand, commandLength, dataReceived,
                          lengthToReceive);
    flashChipSelectHigh();

    return status;
}

/*
 * Function:       Write FLASH Identification
 * Arguments:      regToRead, commandLength
 * Description:    Sends FLASH command
 * Return Message: Bool
 */
static bool flashWrite(uint8_t *command, uint8_t commandLength)
{
    bool status = false;

    flashChipSelectLow();
    status = SPI_Transfer(MX25_FLASH, command, commandLength, NULL,
                          0);
    flashChipSelectHigh();

    return status;
}

/*
 * Function:       flashSendAddr
 * Arguments:      flashAddress, 32 bit flash memory address
 *                 io_mode, I/O mode to transfer address
 *                 addr4ByteMode,
 * Description:    Send flash address with 3-byte or 4-byte mode.
 *                  Assumes that the chip select is already low when calling this
 * Return Message: None
 */
static bool flashSendAddr(uint32_t flashAddress, bool addr4ByteMode)
{
    uint8_t length = 3;
    uint8_t address[4];
    bool status = false;

    /* Check flash is 3-byte or 4-byte mode.
       4-byte mode: Send 4-byte address (A31-A0)
       3-byte mode: Send 3-byte address (A23-A0) */
    if (addr4ByteMode == true)
    {
        length = 4;
        address[0] = flashAddress >> 24;
        address[1] = flashAddress >> 16;
        address[2] = flashAddress >> 8;
        address[3] = flashAddress;
    }
    else
    {
        address[0] = flashAddress >> 16;
        address[1] = flashAddress >> 8;
        address[2] = flashAddress;
    }

    status = SPI_Transfer(MX25_FLASH, address, length, NULL, 0);

    return status;
}

/*
 * Function:       flashIsBusy
 * Arguments:      None.
 * Description:    Check status register WIP bit.
 *                 If  WIP bit = 1: return true ( Busy )
 *                             = 0: return false ( Ready ).
 * Return Message: true, false
 */
static bool flashIsBusy(void)
{
    uint8_t dataBuffer;

    MX25_RDSR(&dataBuffer);
    if ((dataBuffer & FLASH_WIP_MASK) == FLASH_WIP_MASK)
        return true;
    else
        return false;
}

/*
 * Function:       flashWaitTillReady
 * Arguments:      expectTimeMs, expected time-out value of flash operations in ms.
 *                 No use at non-synchronous IO mode.
 * Description:    Synchronous IO:
 *                 If flash is ready return true.
 *                 If flash is time-out return FALSE.
 *                 Non-synchronous IO:
 *                 Always return true
 * Return Message: true, false
 */
static bool flashWaitTillReady(uint32_t expectTimeMs)
{
#ifndef NON_SYNCHRONOUS_IO
    uint32_t startTime = xTaskGetTickCount();
    uint32_t timeoutTicks = pdMS_TO_TICKS(expectTimeMs);

    while (flashIsBusy())
    {
        vTaskDelay(pdMS_TO_TICKS(1));
        if ((xTaskGetTickCount() - startTime) > timeoutTicks)
        {
            return false;
        }
    }
    return true;
#else
    return true;
#endif
}

/*** Public  Functions ***/

/*
 * Function:       MX25_RDID
 * Arguments:      identification, 32 bit buffer to store id
 * Description:    The RDID instruction is to read the manufacturer ID
 *                 of 1-byte and followed by Device ID of 2-byte.
 * Return Message: FLASH_OPERATION_SUCCESS, FLASH_OPERATION_FAILED if failed
 */
flashReturnMsg_t MX25_RDID(uint32_t *identification)
{
    uint32_t tempBuffer;
    uint8_t rdidCmd = FLASH_CMD_RDID;
    bool status = false;
    uint8_t dataBuffer[3];

    // SPI transfer RDID command
    status = flashRead(&rdidCmd, sizeof(rdidCmd), dataBuffer, sizeof(dataBuffer));

    if (status)
    {
        printf("MX25 Manuf ID: %X\n", dataBuffer[0]);
        printf("MX25 Memory Type: %X\n", dataBuffer[1]);
        printf("MX25 Memory Density: %X\n", dataBuffer[2]);

        // Store identification
        tempBuffer = dataBuffer[0];
        tempBuffer = (tempBuffer << 8) | dataBuffer[1];
        *identification = (tempBuffer << 8) | dataBuffer[2];

        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_RES
 * Arguments:      electronicIdentification, 8 bit buffer to store electric id
 * Description:    The RES instruction is to read the Device
 *                 electric identification of 1-byte.
 * Return Message: FLASH_OPERATION_SUCCESS, FLASH_OPERATION_FAILED if failed
 */
flashReturnMsg_t MX25_RES(uint8_t *electronicIdentification)
{
    uint8_t resCmd = FLASH_CMD_RES;
    bool status = false;

    // Chip Select Low
    flashChipSelectLow();

    // SPI transfer RES command
    status = SPI_Transfer(MX25_FLASH, (uint8_t *)&resCmd, sizeof(resCmd), NULL,
                          0);

    if (status)
    {
        // Insert 3 dummy cycles
        status = flashInsertDummyCycle(3);
    }

    if (status)
    {
        // Receive Electronic ID
        SPI_Transfer(MX25_FLASH, NULL, 0, electronicIdentification,
                     sizeof(electronicIdentification));
    }

    // Chip Select High
    flashChipSelectHigh();

    if (status)
    {
        printf("MX25 Electronic Identification: %X\n", *electronicIdentification);
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_REMS
 * Arguments:      remsIdentification, 16 bit buffer to store id
 *                 fsptr, pointer of flash status structure
 * Description:    The REMS instruction is to read the Device
 *                 manufacturer ID and electric ID of 1-byte.
 * Return Message: FLASH_OPERATION_SUCCESS, FLASH_OPERATION_FAILED if failed
 */
flashReturnMsg_t MX25_REMS(uint16_t *remsIdentification, FlashStatus_t *fsptr)
{
    uint8_t remsCmd = FLASH_CMD_REMS;
    bool status = false;
    uint8_t dataBuffer[2];

    // Chip Select Low
    flashChipSelectLow();

    // SPI transfer REMS command
    status = SPI_Transfer(MX25_FLASH, (uint8_t *)&remsCmd, sizeof(remsCmd), NULL,
                          0);

    if (status)
    {
        // Insert 2 dummy cycles
        status = flashInsertDummyCycle(2);
    }

    if (status)
    {
        // Send flash command for data arrange option and receive REMS ID
        // ArrangeOpt = 0x00 will output the manufacturer's ID first
        //            = 0x01 will output electric ID first
        status = SPI_Transfer(MX25_FLASH, (uint8_t *)&fsptr->ArrangeOpt, sizeof(fsptr->ArrangeOpt), dataBuffer,
                              sizeof(dataBuffer));
    }

    // Chip Select High
    flashChipSelectHigh();

    if (status)
    {
        *remsIdentification = (dataBuffer[0] << 8) | dataBuffer[1];
        printf("MX25 Manuf ID and Device ID: %X\n", *remsIdentification);
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_RDSCUR
 * Arguments:      securityReg, 8 bit buffer to store security register value
 * Description:    The RDSCUR instruction is for reading the value of
 *                 Security Register bits.
 * Return Message: FLASH_OPERATION_SUCCESS, FLASH_OPERATION_FAILED if failed
 */
flashReturnMsg_t MX25_RDSCUR(uint8_t *securityReg)
{
    uint8_t dataBuffer;
    uint8_t rdscurCmd = FLASH_CMD_RDSCUR;
    bool status = false;

    if (securityReg == NULL)
    {
        return FLASH_OPERATION_FAILED;
    }

    status = flashRead(&rdscurCmd, sizeof(rdscurCmd), &dataBuffer, sizeof(dataBuffer));

    if (status)
    {
        *securityReg = dataBuffer;
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_READ
 * Arguments:      flashAddress, 32 bit flash memory address
 *                 targetAddress, buffer address to store returned data
 *                 byteLength, length of returned data in byte unit
 * Description:    The READ instruction is for reading data out.
 * Return Message: FLASH_ADDRESS_INVALID, FLASH_OPERATION_SUCCESS, FLASH_OPERATION_FAILED
 */
flashReturnMsg_t MX25_READ(uint32_t flashAddress, uint8_t *targetAddress, uint32_t byteLength)
{
    uint8_t addr4ByteMode;
    uint8_t readCmd = FLASH_CMD_READ;
    bool status = false;

    // Check flash address
    if (flashAddress > FLASH_SIZE)
        return FLASH_ADDRESS_INVALID;

    // Check 3-byte or 4-byte mode
    if (flashIs4Byte())
        addr4ByteMode = true; // 4-byte mode
    else
        addr4ByteMode = false; // 3-byte mode

    // Chip Select Low
    flashChipSelectLow();

    // Write READ command and address
    status = SPI_Transfer(MX25_FLASH, (uint8_t *)&readCmd, sizeof(readCmd), NULL,
                          0);

    if (status)
    {
        // Send flash address
        status = flashSendAddr(flashAddress, addr4ByteMode);
    }

    if (status)
    {
        // Receive data from target address
        SPI_Transfer(MX25_FLASH, NULL, 0, targetAddress,
                     byteLength);
    }

    // Chip select go high to end a flash command
    flashChipSelectHigh();

    if (status)
    {
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_RDSR
 * Arguments:      StatusReg, 8 bit buffer to store status register value
 * Description:    The RDSR instruction is for reading Status Register Bits.
 * Return Message: FLASH_OPERATION_SUCCESS, FLASH_OPERATION_FAILED if failed
 */
flashReturnMsg_t MX25_RDSR(uint8_t *StatusReg)
{
    uint8_t dataBuffer;
    uint8_t rdsrCmd = FLASH_CMD_RDSR;
    bool status = false;

    if (StatusReg == NULL)
    {
        return FLASH_OPERATION_FAILED;
    }

    status = flashRead(&rdsrCmd, sizeof(rdsrCmd), &dataBuffer, sizeof(dataBuffer));

    if (status)
    {
        *StatusReg = dataBuffer;
        // printf("MX25 Status Reg: %X\n", *StatusReg);
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_WREN
 * Arguments:      None.
 * Description:    The WREN instruction is for setting
 *                 Write Enable Latch (WEL) bit.
 * Return Message: FLASH_OPERATION_SUCCESS, FLASH_OPERATION_FAILED if failed
 */
flashReturnMsg_t MX25_WREN(void)
{
    uint8_t wrenCmd = FLASH_CMD_WREN;
    bool status = false;

    status = flashWrite((uint8_t *)&wrenCmd, sizeof(wrenCmd));

    if (status)
    {
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_CE
 * Arguments:      None.
 * Description:    The CE instruction is for erasing the data
 *                 of the whole chip to be "1".
 * Return Message: FlashIsBusy, FlashOperationSuccess, FlashTimeOut
 */
flashReturnMsg_t MX25_CE(void)
{
    flashReturnMsg_t msg = FLASH_OPERATION_FAILED;
    bool status = false;
    uint8_t ceCmd = FLASH_CMD_CE;

    // Check flash is busy or not
    if (flashIsBusy())
    {
        return FLASH_IS_BUSY;
    }

    // Setting Write Enable Latch bit
    msg = MX25_WREN();

    if (msg != FLASH_OPERATION_SUCCESS)
    {
        return msg;
    }

    // Chip select go low to start a flash command
    flashChipSelectLow();

    // Write Chip Erase command = 0x60;
    status = SPI_Transfer(MX25_FLASH, &ceCmd, sizeof(ceCmd), NULL,
                          0);

    // Chip select go high to end a flash command
    flashChipSelectHigh();

    if (!status)
    {
        return FLASH_OPERATION_FAILED;
    }

    if (flashWaitTillReady(CHIP_ERASE_CYCLE_TIME))
        return FLASH_OPERATION_SUCCESS;
    else
        return FLASH_TIME_OUT;
}

/*
 * Function:       MX25_SE
 * Arguments:      flashAddress, 32 bit flash memory address
 * Description:    The SE instruction is for erasing the data
 *                 of the chosen sector (4KB) to be "1".
 * Return Message: FLASH_ADDRESS_INVALID, FLASH_IS_BUSY, FLASH_OPERATION_SUCCESS,
 *                 FLASH_TIME_OUT
 */
flashReturnMsg_t MX25_SE(uint32_t flashAddress)
{
    uint8_t addr4ByteMode;
    uint8_t seCmd = FLASH_CMD_SE;
    bool status = false;

    // Check flash address
    if (flashAddress > FLASH_SIZE)
        return FLASH_ADDRESS_INVALID;

    // Check flash is busy or not
    if (flashIsBusy())
        return FLASH_IS_BUSY;

    // Check 3-byte or 4-byte mode
    if (flashIs4Byte())
        addr4ByteMode = true; // 4-byte mode
    else
        addr4ByteMode = false; // 3-byte mode

    // Setting Write Enable Latch bit
    if (MX25_WREN() != FLASH_OPERATION_SUCCESS)
    {
        return FLASH_TIME_OUT;
    }

    // Chip select go low to start a flash command
    flashChipSelectLow();

    // Write Sector Erase command = 0x20;
    status = SPI_Transfer(MX25_FLASH, &seCmd, sizeof(seCmd), NULL,
                          0);

    if (status)
    {
        status = flashSendAddr(flashAddress, addr4ByteMode);
    }

    // Chip select go high to end a flash command
    flashChipSelectHigh();

    if (!status)
    {
        return FLASH_OPERATION_FAILED;
    }

    if (flashWaitTillReady(SECTOR_ERASE_CYCLE_TIME))
    {
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_TIME_OUT;
    }
}

/*
 * Function:       MX25_PP
 * Arguments:      flashAddress, 32 bit flash memory address
 *                 sourceAddress, buffer address of source data to program
 *                 byteLength, byte length of data to programm
 * Description:    The PP instruction is for programming
 *                 the memory to be "0".
 *                 The device only accept the last 256 byte ( or 32 byte ) to program.
 *                 If the page address ( flashAddress[7:0] ) reach 0xFF, it will
 *                 program next at 0x00 of the same page.
 *                 Some products have smaller page size ( 32 byte )
 * Return Message: FLASH_ADDRESS_INVALID, FLASH_IS_BUSY, FLASH_OPERATION_SUCCESS,
 *                 FLASH_TIME_OUT
 */
flashReturnMsg_t MX25_PP(uint32_t flashAddress, uint8_t *sourceAddress, uint32_t byteLength)
{
    uint8_t addr4ByteMode;
    uint8_t ppCmd = FLASH_CMD_PP;
    bool status = false;

    // Check flash address
    if (flashAddress > FLASH_SIZE)
        return FLASH_ADDRESS_INVALID;

    // Check flash is busy or not
    if (flashIsBusy())
        return FLASH_IS_BUSY;

    // Check 3-byte or 4-byte mode
    if (flashIs4Byte())
        addr4ByteMode = true; // 4-byte mode
    else
        addr4ByteMode = false; // 3-byte mode

    // Setting Write Enable Latch bit
    if (MX25_WREN() != FLASH_OPERATION_SUCCESS)
    {
        return FLASH_TIME_OUT;
    }

    // Chip select go low to start a flash command
    flashChipSelectLow();

    // Write Sector Erase command = 0x20;
    status = SPI_Transfer(MX25_FLASH, &ppCmd, sizeof(ppCmd), NULL,
                          0);

    if (status)
    {
        flashSendAddr(flashAddress, addr4ByteMode);
    }

    if (status)
    {
        status = SPI_Transfer(MX25_FLASH, sourceAddress, byteLength, NULL,
                              0);
    }

    // Chip select go high to end a flash command
    flashChipSelectHigh();

    if (!status)
    {
        return FLASH_TIME_OUT;
    }

    // Wait for flash to reset busy flag
    if (flashWaitTillReady(PAGE_PROGRAM_CYCLE_TIME))
        return FLASH_OPERATION_SUCCESS;
    else
        return FLASH_TIME_OUT;
}

/*
 * Function:       MX25_DP
 * Arguments:      None.
 * Description:    The Deep Power Down instruction is for setting the
 *                 device on the minimizing the power consumption.
 * Return Message: None.
 */
flashReturnMsg_t MX25_DP(void)
{
    uint8_t dpCmd = FLASH_CMD_DP;
    bool status = false;

    status = flashWrite((uint8_t *)&dpCmd, sizeof(dpCmd));

    // Give the device time to transition from standby mode to power down mode
    vTaskDelay(pdMS_TO_TICKS(STANDBY_TO_DP_MODE_DELAY));

    if (status)
    {
        return FLASH_OPERATION_SUCCESS;
    }
    else
    {
        return FLASH_OPERATION_FAILED;
    }
}

/*
 * Function:       MX25_WAKE
 * Arguments:      None.
 * Description:    The wake up function sets CS pin low to wake up flash from deep power down
 * Return Message: None.
 */
void MX25_WAKE(void)
{
    // Clear CS
    flashChipSelectLow();

    // Wake the device by leaving CS low
    vTaskDelay(pdMS_TO_TICKS(WAKE_UP_CS_PIN_LOW_TIME));

    // Set CS
    flashChipSelectHigh();

    // Give the device time to transition from power down mode to standby mode
    vTaskDelay(pdMS_TO_TICKS(DP_TO_STANDBY_MODE_DELAY));
}

/*
 * Function:       MX25_Init
 * Arguments:      None
 * Description:    Initialize Chip Select pin for MX25 flash,
 *                  Set Chip Select pin high
 * Return Message: true
 */
bool MX25_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Init FLASH CS pin
    GPIO_ClockEnable(PORT(FLASH_CS));
    GPIO_InitStruct.Pin = PIN(FLASH_CS);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(PORT(FLASH_CS), &GPIO_InitStruct);

    // Set CS pin high (Set it low during SPI comm)
    flashChipSelectHigh();

    return true;
}
