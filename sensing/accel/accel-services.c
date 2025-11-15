/*
================================================================================================#=
FILE:
accel-services.c

DESCRIPTION:
    The Accel_Serv module provides services to read accelerometer.
    This file implements those services.

Adaptations Notes:  This accel services module was adapted to communicate
                    with the LIS3DSH accelerometer.

Copyright 2022-2023 Twisthink, INC.
This code is licensed under Twisthink license.
See Twisthink-SoftwareLicenseAgreement-Auris.docx for details.
================================================================================================#=
*/

#include "accel-services-api.h"

// FreeRTOS Includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <stdio.h>

/*** Private Constants ***/

#define WAIT_200MS_BETWEEN_POLLS 200
#define ACCEL_CHECK_IN_INTERVAL_MS 5000
#define ACCEL_STACK_SIZE_IN_WORDS 1024

/*** Private Variables ***/
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// TASK MEMORY
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
static StackType_t xAccelTaskStack[ACCEL_STACK_SIZE_IN_WORDS];
static StaticTask_t xAccelTaskControlBlock;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Internal Private Data
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
static SemaphoreHandle_t xBinarySemToSignalAccelTask = NULL;
static StaticSemaphore_t xBinarySemControlBlock;

/*** Private Functions ***/

// -----------------------------------------------------------------------------+-
// Wait for semaphore to be given which signals data is available.
// Once given, parse command and dispatch same.
// -----------------------------------------------------------------------------+-
static void accelServTaskCode(void *arg)
{
    bool result = false;

    result = LIS3DSH_Init();

    if (result == false)
    {
        printf("Failed to init ACCEL driver");
    }
    else
    {
        printf("ACCEL Init Complete\n");
    }

    for (;;)
    {
        // Wait forever for someone to signal us.
        if (xSemaphoreTake(xBinarySemToSignalAccelTask,
                           pdMS_TO_TICKS(ACCEL_CHECK_IN_INTERVAL_MS) == pdTRUE))
        {
            if (LIS3DSH_IsModuleInitialized())
            {
                LIS3DSH_ReadAccelData();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(WAIT_200MS_BETWEEN_POLLS));
    }
}

/*** Public Functions ***/
/*
 * Function:       Set AccelDataReady flag
 * Arguments:      void
 * Description:    Called by interrupt handler
 * Return Message: void
 */
void AccelServ_InterruptHandler(void)
{
    xSemaphoreGiveFromISR(xBinarySemToSignalAccelTask, pdFALSE);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Init the accel services module
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void AccelServ_Init(void)
{
    // ---------------------------------------------------------------------+-
    // Inter-task Coordination:
    // We use a semaphore to block the accel task and a callback to wake-up up the task
    // when accel interrupt is enabled
    // ---------------------------------------------------------------------+-
    xBinarySemToSignalAccelTask = xSemaphoreCreateBinaryStatic(
        &xBinarySemControlBlock);

    // ---------------------------------------------------------------------+-
    // Create accel task to receive and dispatch commands.
    // ---------------------------------------------------------------------+--
    const char *const accelTaskName = "accel";
    void *accelTaskNoParams = NULL;
    UBaseType_t accelTaskPriority = tskIDLE_PRIORITY + 1;

    xTaskCreateStatic(accelServTaskCode, accelTaskName, ACCEL_STACK_SIZE_IN_WORDS,
                      accelTaskNoParams, accelTaskPriority, xAccelTaskStack,
                      &xAccelTaskControlBlock);
}
