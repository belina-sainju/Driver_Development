#pragma once

/*
 * gpio.h
 *
 *  Created on: Nov 1, 2024
 *      Author: Belina Sainju
 */

#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal.h"

// =============================================================================================#=
// GPIO peripheral clock enable
//
// Arguments: The GPIO port enable the appropriate clock.
//
// Returns None
// =============================================================================================#=
void GPIO_ClockEnable(GPIO_TypeDef *port);
