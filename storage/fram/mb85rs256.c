/*
 * mb85rs256.c
 *
 *  Created on: Nov 1, 2024
 *      Author: Belina Sainju
 */

#include "mb85rs256.h"

#include "board-model.h"
#include "../platform/gpio/gpio.h"
#include "../platform/spi/spi-core.h"

#include <string.h> //  for use of memcpy

/*
    FRAM Commands
*/
#define FRAM_OPCODE_WREN 0x06  // Set Write Enable Latch
#define FRAM_OPCODE_WRDI 0x04  // Reset Write Enable Latch
#define FRAM_OPCODE_RDSR 0x05  // Read Status Register
#define FRAM_OPCODE_WRSR 0x01  // Write Status Register
#define FRAM_OPCODE_READ 0x03  // Read Memory Code
#define FRAM_OPCODE_WRITE 0x02 // Write Memory Code
#define FRAM_OPCODE_RDID 0x9F  // Read Device ID
#define FRAM_OPCODE_SLEEP 0xB9 // Sleep Mode

#define FRAM_ADDRESS_LENGTH_IN_BYTES 2
#define FRAM_OP_CODE_LENGTH_IN_BYTES 1

/*** Private  Functions ***/

// Helper function to set CS pin high
static void framChipSelectHigh(void)
{
    HAL_GPIO_WritePin(PORT(FRAM_CS), PIN(FRAM_CS), GPIO_PIN_SET);
}

// Helper function to set CS pin low
static void framChipSelectLow(void)
{
    HAL_GPIO_WritePin(PORT(FRAM_CS), PIN(FRAM_CS), GPIO_PIN_RESET);
}

/*
 * Function:       Read FRAM Register
 * Arguments:      command, *dataReceived, lengthToReceive
 * Description:    Sends FRAM command and stores data read in dataReceived
 * Return Message: Bool
 */
static bool framReadRegister(uint8_t command, uint8_t *dataReceived, uint16_t lengthToReceive)
{
    bool status = false;

    framChipSelectLow();
    status = SPI_Transfer(MB85_FRAM, &command, FRAM_OP_CODE_LENGTH_IN_BYTES, dataReceived,
                          lengthToReceive);
    framChipSelectHigh();

    return status;
}

/*
 * Function:       Write FRAM Command/OpCode
 * Arguments:      command
 * Description:    Sends FRAM command
 * Return Message: Bool
 */
static bool framSendCommand(uint8_t command)
{
    bool status = false;

    framChipSelectLow();
    status = SPI_Transfer(MB85_FRAM, &command, FRAM_OP_CODE_LENGTH_IN_BYTES, NULL,
                          0);
    framChipSelectHigh();

    return status;
}

/*** Public  Functions ***/

/*
 * Function:       Read FRAM
 * Arguments:      readAddress, *dataReceived, lengthToReceive
 * Description:    Sends FRAM READ opCode, readAddress, and stores data read in dataReceived
 * Return Message: true, false if failed
 */
bool MB85RS256_Read(uint16_t readAddress, uint8_t *dataReceived, uint16_t lengthToReceive)
{
    bool status = false;
    uint8_t readOpCode = FRAM_OPCODE_READ;
    uint8_t readAddressToSend[FRAM_ADDRESS_LENGTH_IN_BYTES];
    readAddressToSend[0] = (readAddress >> 8) & 0xFF; // MSB
    readAddressToSend[1] = (readAddress) & 0xFF;      // LSB

    // Check the number of bytes to read
    if (lengthToReceive > FRAM_SIZE_IN_BYTES)
    {
        lengthToReceive = FRAM_SIZE_IN_BYTES;
    }

    framChipSelectLow();

    status = SPI_Transfer(MB85_FRAM, &readOpCode, sizeof(readOpCode), NULL,
                          0);
    if (status)
    {
        status = SPI_Transfer(MB85_FRAM, readAddressToSend, FRAM_ADDRESS_LENGTH_IN_BYTES, NULL,
                              0);
        if (status)
        {
            status = SPI_Transfer(MB85_FRAM, NULL, 0, dataReceived,
                                  lengthToReceive);
        }
    }

    framChipSelectHigh();

    return status;
}

/*
 * Function:       Write FRAM
 * Arguments:      writeAddress, *dataToWrite, lengthToSend
 * Description:    Sends FRAM WRITE opCode, writeAddress and dataToWrite
 * Return Message: true, false if failed
 */
bool MB85RS256_Write(uint16_t writeAddress, uint8_t *dataToWrite, uint16_t lengthToSend)
{
    bool status = false;
    uint8_t writeOpCode = FRAM_OPCODE_WRITE;
    uint8_t writeAddressToSend[FRAM_ADDRESS_LENGTH_IN_BYTES];
    writeAddressToSend[0] = (writeAddress >> 8) & 0xFF; // MSB
    writeAddressToSend[1] = (writeAddress) & 0xFF;      // LSB

    // Check the number of bytes to write
    if (lengthToSend > FRAM_SIZE_IN_BYTES)
    {
        lengthToSend = FRAM_SIZE_IN_BYTES;
    }

    // Set Write Enable Latch before writing to FRAM
    status = MB85RS256_WREN();
    if (!status)
    {
        printf("Failed to set WREN before writing to FRAM");
        return status;
    }

    framChipSelectLow();

    status = SPI_Transfer(MB85_FRAM, &writeOpCode, sizeof(writeOpCode), NULL,
                          0);

    if (status)
    {
        status = SPI_Transfer(MB85_FRAM, writeAddressToSend, FRAM_ADDRESS_LENGTH_IN_BYTES, NULL,
                              0);

        if (status)
        {
            status = SPI_Transfer(MB85_FRAM, dataToWrite, lengthToSend, NULL, 0);
        }
    }

    framChipSelectHigh();

    status = MB85RS256_WRDI();
    if (!status)
    {
        printf("Failed to reset Write Enable Latch");
    }

    return status;
}

/*
 * Function:       Read FRAM Status Register
 * Arguments:      statusRegValue
 * Description:    Sends FRAM RDSR opCode and stores register value in dataReceived
 * Return Message: true, false if failed
 */
bool MB85RS256_RDSR(uint8_t *statusRegValue)
{
    bool status = false;

    status = framReadRegister(FRAM_OPCODE_RDSR, statusRegValue, sizeof(statusRegValue));

    return status;
}

/*
 * Function:       MB85RS256_WREN
 * Arguments:      None.
 * Description:    The WREN instruction is for setting
 *                 Write Enable Latch (WEL) bit.
 * Return Message: true, false if failed
 */
bool MB85RS256_WREN(void)
{
    bool status = false;

    status = framSendCommand(FRAM_OPCODE_WREN);

    return status;
}

/*
 * Function:       MB85RS256_WRDI
 * Arguments:      None.
 * Description:    The WRDI instruction is for resetting
 *                 Write Enable Latch (WEL) bit.
 * Return Message: true, false if failed
 */
bool MB85RS256_WRDI(void)
{
    bool status = false;

    status = framSendCommand(FRAM_OPCODE_WRDI);

    return status;
}

/*
 * Function:       MB85RS256_RDID
 * Arguments:      identification, 32 bit buffer to store id
 * Description:    The RDID instruction is to read the manufacturer ID
 *                 of 1-byte and followed by Continuation code of 1-byte,
 *                 followed by Product ID of 2-byte
 * Return Message: true, false if failed
 */
bool MB85RS256_RDID(uint32_t *identification)
{
    uint32_t temp;
    bool status = false;
    uint8_t dataBuffer[4];

    // SPI transfer RDID command
    status = framReadRegister(FRAM_OPCODE_RDID, dataBuffer, sizeof(dataBuffer));

    if (status)
    {
        printf("MB85RS256 Manuf ID: %X\n", dataBuffer[0]);
        printf("MB85RS256 Continuation code: %X\n", dataBuffer[1]);
        printf("MB85RS256 Product ID (1st Byte): %X\n", dataBuffer[2]);
        printf("MB85RS256 Product ID (2nd Byte): %X\n", dataBuffer[3]);

        // Store identification
        temp = dataBuffer[0];
        temp = (temp << 8) | dataBuffer[1];
        temp = (temp << 8) | dataBuffer[2];
        *identification = (temp << 8) | dataBuffer[3];
    }

    return status;
}

/*
 * Function:       MB85RS256_Init
 * Arguments:      None
 * Description:    Initialize Chip Select pin for MB85RS256 FRAM,
 *                  Set Chip Select pin high
 * Return Message: true
 */
bool MB85RS256_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Init FRAM CS pin
    GPIO_ClockEnable(PORT(FRAM_CS));
    GPIO_InitStruct.Pin = PIN(FRAM_CS);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(PORT(FRAM_CS), &GPIO_InitStruct);

    // Set CS pin high (Set it low during SPI comm)
    framChipSelectHigh();

    return true;
}
