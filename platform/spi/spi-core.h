#pragma once

/**
 *  @file                   platform/spi/spi-core.h
 *  @brief                  A software module that provides services for communicating
 *                          with slave devices via SPI. This is meant to be a generic
 *                          helper module for HAL SPI receive and transmit operations.
 *  @copyright              Twisthink
 *  @date                   7/2/2024
 *
 *  @remark
 */

#include "stdbool.h"

#include <stdint.h>

typedef enum
{
    LIS3DSH_ACCEL,
    MX25_FLASH,
    MB85_FRAM
} SpiDevice_t;

// =============================================================================================#=
// Public API Functions
// =============================================================================================#=

// =============================================================================================#=
// Send and receive data over SPI. The function supports read or write lengths of 0, assuming the
// opposite operation is populated.
//
// Note: The SPI bus must be acquired successfully prior to calling this function.
// =============================================================================================#=
bool SPI_Transfer(SpiDevice_t device, uint8_t *dataToSend, uint16_t lengthToSend, uint8_t *dataReceived, uint16_t lengthToReceive);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// One-time startup initialization for the spi peripheral and the associated pins
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void SPI_Init(void);
