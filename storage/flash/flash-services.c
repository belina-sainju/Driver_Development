/*
================================================================================================#=
FILE:
flash-services.c

DESCRIPTION:
    The FlashServ module provides services to read, write, and erase flash memory.
    This file implements those services.

Adaptations Notes:  This flash services module was adapted to communicate
                    with the MX25 16Mb (2MB) flash memory.

                    The readable/writable address space is 0 (0x0) to 2048000 (0x001F4000)

                    Read operations can be performed across the full address space starting at
                    any address. The length to read is specified in bytes and can be 1 - infinity
                    (This would wrap around indefinitely).

                    Write operations can be performed accross the full address space starting at
                    any address. However, It is the client's responsibilty to manage it's allocated
                    address space and ensure the provided address for a write operation falls within
                    this allocated address space. (This also applies to block and sector erase operations)
                    The length to write is specified in bytes. A single write operation
                    can only write a maximum of 256 bytes. However, this is managed
                    within flash-services. This means a client CAN provide a write buffer
                    larger than 256 bytes in size

                    A write operation requires that the region of memory being written first be erased.
                    The Flash Services API defines three different region sizes performing erase operations.
                    These are, largest to smallest, Block, Page, and Sector. The API also provides an 'erase
                    all' option to erase the full flash memory.

                    Due to restrictions in the Macronix MX25V driver, this implementation of Flash Services
                    only supports two region types: Block and Sector.

                    The erase operation allows the caller to erase any region (Block or Sector)
                    across the full address space. The caller specifies the region to be erased by
                    providing the start address of that region. Unless the caller requests EraseAll,
                    at most one region may be erased at a time.

                    For this Macronix MX25V specific implementation:
                        a BLOCK is an MX25V 64KB Sector and
                        a SECTOR is an MX25V 4kB Subsector.

Copyright 2022-2023 Twisthink, INC.
This code is licensed under Twisthink license.
See Twisthink-SoftwareLicenseAgreement-Auris.docx for details.
================================================================================================#=
*/

#include "flash-services-api.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdlib.h>

#define FLASH_TARGET_ADDR 0x00000000
#define RANDOM_SEED 106
#define TRANS_LENGTH 16

#define FLASH_EXERCISE_BYTES 64
#define FLASH_EXERCISE_CYCLES 0x2000 // 2MB/256bytesPerPage = 8192 pages

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// TASK MEMORY
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
#define FLASH_STACK_SIZE_IN_WORDS 1024
static StackType_t xFlashTaskStack[FLASH_STACK_SIZE_IN_WORDS];
static StaticTask_t xFlashTaskControlBlock;

/*** Private Functions ***/

/*
 * Simple flash ID test
 */
static bool flashIdTest(void)
{
    uint32_t flashId = 0;
    uint8_t resId = 0;
    uint16_t remsId = 0;

    flashReturnMsg_t msg = FLASH_WRITE_REG_FAILED;

    // Read Manufacturer ID, Memory Type, and Memory Density
    msg = MX25_RDID(&flashId);

    if (msg != FLASH_OPERATION_SUCCESS)
    {
        return false;
    }
    else
    {
        if (flashId != FLASH_DEVICE_ID)
        {
            printf("Flash ID invalid.\n");
        }
    }

    // Read Electronic ID
    msg = MX25_RES(&resId);
    if (msg != FLASH_OPERATION_SUCCESS)
    {
        return false;
    }
    else
    {
        if (resId != ELECTRONIC_ID)
        {
            printf("Electronic ID invalid.\n");
        }
    }

    // Read Manufacturer ID and Device ID
    /* Decide remsId order. 0: { manufacturer id, device id }
                             1: { device id,  manufacturer id } */
    FlashStatus_t flashState = {0};
    flashState.ArrangeOpt = 0;
    msg = MX25_REMS(&remsId, &flashState);
    if (msg != FLASH_OPERATION_SUCCESS)
    {
        return false;
    }
    else
    {
        if (flashState.ArrangeOpt)
        {
            if (remsId != REMS_ID_1)
            {
                printf("REMS ID invalid.\n");
                return false;
            }
        }
        else
        {
            if (remsId != REMS_ID_0)
            {
                printf("REMS ID invalid.\n");
                return false;
            }
        }
    }

    return true;
}

/*
 * Simple flash read/write test
 */
static bool flashReadWriteTest(void)
{
    flashReturnMsg_t msg = FLASH_WRITE_REG_FAILED;

    uint32_t flashAddr = FLASH_TARGET_ADDR;
    uint32_t transLen = TRANS_LENGTH;
    uint8_t memoryAddr[TRANS_LENGTH] = {0};
    uint8_t memoryAddrCmp[TRANS_LENGTH] = {0};

    uint16_t i = 0;

    // Generate data to write to flash
    // Seed the random number generator
    srand(RANDOM_SEED);
    for (i = 0; i < transLen; i = i + 1)
    {
        // Generate random byte data
        memoryAddr[i] = rand() % 256;
    }

    // Erase 4K sector of flash memory
    msg = MX25_SE(flashAddr);
    if (msg != FLASH_OPERATION_SUCCESS)
    {
        printf("Failed to erase flash memory.\n");
        return false;
    }

    // Program data to flash memory
    msg = MX25_PP(flashAddr, memoryAddr, transLen);
    if (msg != FLASH_OPERATION_SUCCESS)
    {
        printf("Failed to program flash memory.\n");
        return false;
    }

    msg = MX25_READ(flashAddr, memoryAddrCmp, transLen);
    if (msg != FLASH_OPERATION_SUCCESS)
    {
        printf("Failed to read flash memory.\n");
        return false;
    }

    // Compare original data and flash data
    for (i = 0; i < (transLen); i = i + 1)
    {
        if (memoryAddr[i] != memoryAddrCmp[i])
        {
            printf("Data written to flash memory doesn't match data read.\n");
            return false;
        }
    }

    // Erase 4K sector of flash memory
    msg = MX25_SE(flashAddr);
    if (msg != FLASH_OPERATION_SUCCESS)
    {
        printf("Failed to reset flash memory.\n");
        return false;
    }

    return true;
}

/*
 * Flash exercise to write to all pages and read data back to confirm values
 */
static bool flashReadWriteExercise(void)
{
    uint32_t dataLength = FLASH_EXERCISE_BYTES;
    uint32_t index = 0;
    flashReturnMsg_t msg = FLASH_OPERATION_FAILED;
    uint8_t readBuffer[FLASH_EXERCISE_BYTES] = {0};
    uint32_t startAddress = 0;
    uint8_t writeBuffer[FLASH_EXERCISE_BYTES] = {0};

    // Fill up the write buffer
    for (index = 0; index < FLASH_EXERCISE_BYTES; index++)
    {
        writeBuffer[index] = index;
    }

    // Bulk erase
    printf("Erasing flash\n");
    msg = MX25_CE();
    if (msg != FLASH_OPERATION_SUCCESS)
    {
        printf("Failed to Chip Erase.\n");
        return false;
    }
    printf("Erase flash complete");

    // Cycle through all of flash
    for (index = 0; index < FLASH_EXERCISE_CYCLES; index++)
    {
        // Assign memory address
        startAddress = index * FLASH_EXERCISE_BYTES;

        // Write
        msg = MX25_PP(startAddress, writeBuffer, dataLength);
        if (msg != FLASH_OPERATION_SUCCESS)
        {
            printf("Failed to program flash at cycle %ld\n", index);
            return false;
        }

        // Delay
        vTaskDelay(pdMS_TO_TICKS(5));

        // Read
        msg = MX25_READ(startAddress, readBuffer, dataLength);
        if (msg != FLASH_OPERATION_SUCCESS)
        {
            printf("Failed to read flash at cycle %ld\n", index);
            return false;
        }

        // Delay
        vTaskDelay(pdMS_TO_TICKS(3));

        // Compare
        printf("Flash Exercise Cycle: %lu\n", index);
        for (uint32_t i = 0; i < FLASH_EXERCISE_BYTES; i++)
        {
            if (readBuffer[i] != i)
            {
                printf("Flash test failed: Unexpected byte at index %ld\n", i);
                return false;
            }
        }
    }

    return true;
}

// -----------------------------------------------------------------------------+-
// Wait for semaphore to be given which signals data is available.
// Once given, parse command and dispatch same.
// -----------------------------------------------------------------------------+-
static void flashTaskCode(void *arg)
{
    bool result = false;

    // Initialize flash
    result = MX25_Init();

    if (result == false)
    {
        printf("Failed to init flash driver");
    }
    else
    {
        printf("Flash Init Complete\n");
    }

    // Warm up time delay
    vTaskDelay(pdMS_TO_TICKS(FLASH_FULL_ACCESS_TIME));

    // Test Flash ID
    result = flashIdTest();
    if (result == false)
    {
        printf("Flash ID test failed.\n");
    }
    else
    {
        printf("Flash ID test passed.\n");
    }

    result = flashReadWriteTest();
    if (result == false)
    {
        printf("Flash Read Write test failed.\n");
    }
    else
    {
        printf("Flash Read Write test passed.\n");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    result = flashReadWriteExercise();
    if (result == false)
    {
        printf("Flash Read Write Exercise failed.\n");
    }
    else
    {
        printf("Flash Read Write Exercise passed.\n");
    }

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/*** Public Functions ***/

/*
 * Function to set flash to DeepPowerDown mode
 */
void FlashServ_LowPowerMode(void)
{
    MX25_DP();
}

/*
 * Function to wake up the flash from DeepPowerDown mode to StandBy mode
 */
void FlashServ_WakeFromLowPowerMode(void)
{
    MX25_WAKE();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Init the flash services module
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void FlashServ_Init(void)
{
    // ---------------------------------------------------------------------+-
    // Create flash task to receive and dispatch commands.
    // ---------------------------------------------------------------------+--
    const char *const flashTaskName = "flash";
    void *flashTaskNoParams = NULL;
    UBaseType_t flashTaskPriority = tskIDLE_PRIORITY + 1;

    xTaskCreateStatic(flashTaskCode, flashTaskName, FLASH_STACK_SIZE_IN_WORDS,
                      flashTaskNoParams, flashTaskPriority, xFlashTaskStack,
                      &xFlashTaskControlBlock);
}
