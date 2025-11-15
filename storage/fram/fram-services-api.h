#pragma once
/*
================================================================================================#=
FILE:
fram-services-api.h

DESCRIPTION:
    The FRAM Serv module provides services to read, write, and erase fram memory.
    This file defines the API to access those services.

Copyright 2023-2024 Twisthink, INC.
This code is licensed under Twisthink license.
See Twisthink-SoftwareLicenseAgreement-Auris.docx for details.
================================================================================================#=
*/

#include <stdint.h>
#include <stdbool.h>

// FLASH information other modules may need to access:
#include "mb85rs256.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Does the needful to initialize the module.
// This should be called only once.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void FramServ_Init(void);
