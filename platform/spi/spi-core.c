/**
 *  @file                   spi-core.c
 *  @brief                  A software module that provides services for communicating
 *                          with slave devices via SPI
 *  @copyright              2024 Stryker Medical
 *  @date                   7/2/2024
 *
 *  @remark
 */

#include "spi-core.h"

// Project Dependencies
#include "../../board/board-model.h"
#include "../gpio/gpio.h"
#include "stm32f4xx_hal_spi.h"

// FreeRTOS Includes
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* Private Defines ----------------------------------------------------------*/
#define SPI_TIMEOUT_MS 1000

/* Private Variables ----------------------------------------------------------*/
// ACCEL SPI variables
static SPI_HandleTypeDef xAccelSpiHandle;

// SHARED SPI for FLASH and FRAM variables
static SPI_HandleTypeDef xSharedSpiHandle;

// SPI mutex to support multiple devices
static StaticSemaphore_t xSPIMutexControlBlock;
static SemaphoreHandle_t xSPIMutex = NULL;

/* Private functions ----------------------------------------------------------*/

// Helper function to acquire shared SPI mutex
static bool spiMutexAcquire(void)
{
    if (xSemaphoreTake(xSPIMutex, pdMS_TO_TICKS(SPI_TIMEOUT_MS)))
    {
        return true;
    }

    return false;
}

// Helper function to release shared SPI mutex
static void spiMutexRelease(void)
{
    xSemaphoreGive(xSPIMutex);
}

static void spi1AccelInit(void)
{
    GPIO_InitTypeDef initStruct = {0};

    GPIO_ClockEnable(PORT(SPI_SCLK));
    GPIO_ClockEnable(PORT(SPI_MISO));

    // Init MOSI, MISO, SCLK pins, CS is initialized in separate modules
    initStruct.Mode = GPIO_MODE_AF_PP;
    initStruct.Speed = GPIO_SPEED_MEDIUM;
    initStruct.Alternate = SPI_ALT_FCN;

    initStruct.Pin = PIN(SPI_SCLK);
    initStruct.Pull = GPIO_PULLUP;
    initStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(PORT(SPI_SCLK), &initStruct);

    initStruct.Pin = PIN(SPI_MISO);
    initStruct.Pull = GPIO_PULLUP;
    initStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(PORT(SPI_MISO), &initStruct);

    initStruct.Pin = PIN(SPI_MOSI);
    initStruct.Pull = GPIO_NOPULL;
    initStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(PORT(SPI_MOSI), &initStruct);

    // Init SPI peripheral
    xAccelSpiHandle.Instance = ACCEL_SPI;
    xAccelSpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 21MHz
    xAccelSpiHandle.Init.Direction = SPI_DIRECTION_2LINES;
    xAccelSpiHandle.Init.Mode = SPI_MODE_MASTER;
    xAccelSpiHandle.Init.CLKPolarity = SPI_POLARITY_HIGH; // CPOL = 1
    xAccelSpiHandle.Init.CLKPhase = SPI_PHASE_2EDGE;      // CPHA = 1
    xAccelSpiHandle.Init.DataSize = SPI_DATASIZE_8BIT;
    xAccelSpiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    xAccelSpiHandle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    xAccelSpiHandle.Init.CRCPolynomial = 0x0;

    // Software controls chip select
    xAccelSpiHandle.Init.NSS = SPI_NSS_SOFT;
    //    xAccelSpiHandle.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    xAccelSpiHandle.Init.TIMode = SPI_TIMODE_DISABLE;

    __HAL_RCC_SPI1_CLK_ENABLE();

    if (HAL_SPI_Init(&xAccelSpiHandle) != HAL_OK)
    {
        /* Initialization Error */
        printf("ACCEL SPI: Init Failed\n");
    }
    else
    {
        printf("ACCEL SPI: Init Complete\n");
    }
}

static void spi2SharedInit(void)
{
    GPIO_InitTypeDef initStruct = {0};

    GPIO_ClockEnable(PORT(SPI2_SCLK));

    // Init MOSI, MISO, SCLK pins, CS is initialized in separate modules
    initStruct.Mode = GPIO_MODE_AF_PP;
    initStruct.Speed = GPIO_SPEED_MEDIUM;
    initStruct.Alternate = SHARED_SPI_ALT_FCN;

    initStruct.Pin = PIN(SPI2_SCLK);
    initStruct.Pull = GPIO_PULLDOWN;
    initStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(PORT(SPI2_SCLK), &initStruct);

    initStruct.Pin = PIN(SPI2_MISO);
    initStruct.Pull = GPIO_PULLUP;
    initStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(PORT(SPI2_MISO), &initStruct);

    initStruct.Pin = PIN(SPI2_MOSI);
    initStruct.Pull = GPIO_PULLDOWN;
    initStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(PORT(SPI2_MOSI), &initStruct);

    // Init SPI peripheral
    xSharedSpiHandle.Instance = SHARED_SPI;
    xSharedSpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 21MHz
    xSharedSpiHandle.Init.Direction = SPI_DIRECTION_2LINES;
    xSharedSpiHandle.Init.Mode = SPI_MODE_MASTER;
    xSharedSpiHandle.Init.CLKPolarity = SPI_POLARITY_LOW; // CPOL = 0
    xSharedSpiHandle.Init.CLKPhase = SPI_PHASE_1EDGE;     // CPHA = 0
    xSharedSpiHandle.Init.DataSize = SPI_DATASIZE_8BIT;
    xSharedSpiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    xSharedSpiHandle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    xSharedSpiHandle.Init.CRCPolynomial = 0x0;

    // Software controls chip select
    xSharedSpiHandle.Init.NSS = SPI_NSS_SOFT;
    //    xSharedSpiHandle.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    xSharedSpiHandle.Init.TIMode = SPI_TIMODE_DISABLE;

    __HAL_RCC_SPI2_CLK_ENABLE();

    if (HAL_SPI_Init(&xSharedSpiHandle) != HAL_OK)
    {
        /* Initialization Error */
        printf("Shared SPI: Init Failed\n");
    }
    else
    {
        printf("Shared SPI: Init Complete\n");
    }
}

/* Public functions ----------------------------------------------------------*/

bool SPI_Transfer(SpiDevice_t device, uint8_t *dataToSend, uint16_t lengthToSend, uint8_t *dataReceived, uint16_t lengthToReceive)
{
    SPI_HandleTypeDef spiHandle;
    HAL_StatusTypeDef stat = HAL_OK;

    if (device == LIS3DSH_ACCEL)
    {
        spiHandle = xAccelSpiHandle;
    }
    else if ((device == MX25_FLASH) || (device == MB85_FRAM))
    {
        spiHandle = xSharedSpiHandle;
    }
    else
    {
        printf("SPI device not recognized.");
        stat = HAL_BUSY;
    }

    if ((stat == HAL_OK) && (spiMutexAcquire()))
    {
        if (lengthToSend > 0)
        {
            stat = HAL_SPI_Transmit(&spiHandle, dataToSend, lengthToSend, SPI_TIMEOUT_MS);
        }

        // This logic requires stat to be initialized as HAL_OK for cases with only
        if (stat == HAL_OK)
        {
            if (lengthToReceive > 0)
            {
                stat = HAL_SPI_Receive(&spiHandle, dataReceived, lengthToReceive, SPI_TIMEOUT_MS);
            }
        }

        if (stat != HAL_OK)
        {
            printf("SPI error %d", stat);
        }

        spiMutexRelease();
    }
    else
    {
        printf("SPI mutex acquire failed.");
        stat = HAL_BUSY;
    }

    return stat == HAL_OK;
}

void SPI_Init(void)
{
    // Init Accelerometer SPI
    spi1AccelInit();

    // Init FLASH and FRAM Shared SPI
    spi2SharedInit();

    // Assign SPIMutex value
    xSPIMutex = xSemaphoreCreateMutexStatic(&xSPIMutexControlBlock);
}
