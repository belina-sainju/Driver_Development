#pragma once
/*
================================================================================================#=
FILE:
flash-services-api.h

DESCRIPTION:
    The FlashServ module provides services to read, write, and erase flash memory.
    This file defines the API to access those services.

Copyright 2023-2024 Twisthink, INC.
This code is licensed under Twisthink license.
See Twisthink-SoftwareLicenseAgreement-Auris.docx for details.
================================================================================================#=
*/

// FLASH information other modules may need to access:
#include "mx25v1635f.h"

#include <stdint.h>
#include <stdbool.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Does the needful to initialize the module.
// This should be called only once.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void FlashServ_Init(void);

// Prep and restore functions for low power mode
void FlashServ_LowPowerMode(void);
void FlashServ_WakeFromLowPowerMode(void);