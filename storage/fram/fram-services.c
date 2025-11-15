/*
================================================================================================#=
FILE:
fram-services.c

DESCRIPTION:
    The FRAM Serv module provides services to read, write, and erase FRAM memory.
    This file implements those services.

Adaptations Notes:  This fram services module was adapted to communicate
                    with the MB85RS256TY 256KBit FRAM memory.

Copyright 2022-2023 Twisthink, INC.
This code is licensed under Twisthink license.
See Twisthink-SoftwareLicenseAgreement-Auris.docx for details.
================================================================================================#=
*/

#include "fram-services-api.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include <stdlib.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// TASK MEMORY
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
#define FRAM_STACK_SIZE_IN_WORDS 1024
static StackType_t xFramTaskStack[FRAM_STACK_SIZE_IN_WORDS];
static StaticTask_t xFramTaskControlBlock;

#define FRAM_TEST_READ_ADDR 0
#define FRAM_TEST_WRITE_LENGTH 10

/*** Private Functions ***/

/*
 * Simple fram read/write test
 */
static bool framTest(void)
{
    uint8_t dataBuffer[FRAM_TEST_WRITE_LENGTH] = {0};
    uint16_t dataLength = FRAM_TEST_WRITE_LENGTH;
    uint8_t readBuffer[FRAM_TEST_WRITE_LENGTH] = {0};
    uint16_t readWriteAddr = FRAM_TEST_READ_ADDR;
    bool result = false;
    uint16_t i = 0;

    for (i = 0; i < dataLength; i = i + 1)
    {
        // Generate data buffer to write to FRAM
        dataBuffer[i] = i + 1;
    }

    result = MB85RS256_Write(readWriteAddr, dataBuffer, dataLength);

    if (result)
    {
        result = MB85RS256_Read(readWriteAddr, readBuffer, dataLength);

        if (result)
        {
            // Compare original data and FRAM data
            for (i = 0; i < (dataLength); i = i + 1)
            {
                if (dataBuffer[i] != readBuffer[i])
                {
                    printf("Data written to fram memory doesn't match data read.\n");
                    return false;
                }
            }
        }
    }
    return result;
}

// -----------------------------------------------------------------------------+-
// Wait for semaphore to be given which signals data is available.
// Once given, parse command and dispatch same.
// -----------------------------------------------------------------------------+-
static void framTaskCode(void *arg)
{
    bool result = false;
    uint32_t framId = 0;

    // Initialize fram
    result = MB85RS256_Init();

    if (result == false)
    {
        printf("Failed to init FRAM driver\n");
    }
    else
    {
        printf("FRAM Init Complete\n");
    }

    result = MB85RS256_RDID(&framId);

    if (result == false)
    {
        printf("Failed to read FRAM ID\n");
    }
    else
    {
        if (framId != FRAM_ID)
        {
            printf("Failed to identify FRAM ID\n");
        }
    }

    result = framTest();

    if (result == false)
    {
        printf("FRAM test failed\n");
    }
    else
    {
        printf("FRAM test passed\n");
    }

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Init the fram services module
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void FramServ_Init(void)
{
    // ---------------------------------------------------------------------+-
    // Create fram task to receive and dispatch commands.
    // ---------------------------------------------------------------------+--
    const char *const framTaskName = "fram";
    void *framTaskNoParams = NULL;
    UBaseType_t framTaskPriority = tskIDLE_PRIORITY + 1;

    xTaskCreateStatic(framTaskCode, framTaskName, FRAM_STACK_SIZE_IN_WORDS,
                      framTaskNoParams, framTaskPriority, xFramTaskStack,
                      &xFramTaskControlBlock);
}
