
#pragma once

#include "net-to-mcu-f407.h"

// -----------------------------------------------------------------------------+-
// SPI
// -----------------------------------------------------------------------------+-

#define SPI_SCLK NET_SPI1_SCLK
#define SPI_MISO NET_SPI1_MISO
#define SPI_MOSI NET_SPI1_MOSI
#define ACCEL_SPI NET_SPI1
#define SPI_ALT_FCN NET_SPI1_ALT_FCN

#define SPI2_SCLK NET_SPI2_SCLK
#define SPI2_MISO NET_SPI2_MISO
#define SPI2_MOSI NET_SPI2_MOSI
#define SHARED_SPI NET_SPI2
#define SHARED_SPI_ALT_FCN NET_SPI2_ALT_FCN

// -----------------------------------------------------------------------------+-
// ACCEL
// -----------------------------------------------------------------------------+-

#define ACCEL_INT1 NET_ACCEL_INT1
#define ACCEL_CS NET_ACCEL_CS

// -----------------------------------------------------------------------------+-
// EXT FLASH
// -----------------------------------------------------------------------------+-

#define FLASH_CS NET_FLASH_CS

// -----------------------------------------------------------------------------+-
// EXT FRAM
// -----------------------------------------------------------------------------+-

#define FRAM_CS NET_FRAM_CS

// -----------------------------------------------------------------------------+-
// PORT & PIN Macros
// Use these in the application code to identify
// the GPIO Port and Pins that are specific to a particular board revision.
//
// X Parameter:
//     This should be one of the board model names for a net
//     as they are defined here in this file.
//
// Note:
// Regarding the extra level of macro definition below...
// this is necessary to force expansion of the board model name.
// This is a common idiom according to stackoverflow...
//     "All of the arguments to a C preprocessor macro are fully expanded before the macro itself is expanded,
//     unless the # or ## operator is applied to them; then they're not expanded. So to get full expansion
//     before ##, you pass the arguments through a wrapper macro that doesn't use ##.
// -----------------------------------------------------------------------------+-
#define PORT(X) __PORT(X)
#define __PORT(X) X##_PORT

#define PIN(X) __PIN(X)
#define __PIN(X) X##_PIN
