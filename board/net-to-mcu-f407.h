#pragma once

#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"

// -----------------------------------------------------------------------------+-
// SPI
// -----------------------------------------------------------------------------+-

#define NET_SPI1_SCLK_PORT GPIOA
#define NET_SPI1_SCLK_PIN GPIO_PIN_5

#define NET_SPI1_MISO_PORT GPIOA
#define NET_SPI1_MISO_PIN GPIO_PIN_6

#define NET_SPI1_MOSI_PORT GPIOA
#define NET_SPI1_MOSI_PIN GPIO_PIN_7

#define NET_SPI1 SPI1
#define NET_SPI1_ALT_FCN GPIO_AF5_SPI1

#define NET_SPI2_SCLK_PORT GPIOB
#define NET_SPI2_SCLK_PIN GPIO_PIN_13

#define NET_SPI2_MISO_PORT GPIOB
#define NET_SPI2_MISO_PIN GPIO_PIN_14

#define NET_SPI2_MOSI_PORT GPIOB
#define NET_SPI2_MOSI_PIN GPIO_PIN_15

#define NET_SPI2 SPI2
#define NET_SPI2_ALT_FCN GPIO_AF5_SPI2

// -----------------------------------------------------------------------------+-
// ACCEL
// -----------------------------------------------------------------------------+-

#define NET_ACCEL_INT1_PORT GPIOE
#define NET_ACCEL_INT1_PIN GPIO_PIN_0

#define NET_ACCEL_CS_PORT GPIOE
#define NET_ACCEL_CS_PIN GPIO_PIN_3

// -----------------------------------------------------------------------------+-
// EXT FLASH
// -----------------------------------------------------------------------------+-

#define NET_FLASH_CS_PORT GPIOB
#define NET_FLASH_CS_PIN GPIO_PIN_12

// -----------------------------------------------------------------------------+-
// EXT FRAM
// -----------------------------------------------------------------------------+-

#define NET_FRAM_CS_PORT GPIOA
#define NET_FRAM_CS_PIN GPIO_PIN_15
