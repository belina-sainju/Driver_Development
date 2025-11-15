#pragma once
/*
================================================================================================#=
FILE:
accel-services-api.h

DESCRIPTION:
    The Accel_Serv module provides services to read accelerometer.
    This file implements those services.

Adaptations Notes:  This accel services module was adapted to communicate
                    with the LIS3DSH accelerometer.

Copyright 2023-2024 Twisthink, INC.
This code is licensed under Twisthink license.
See Twisthink-SoftwareLicenseAgreement-Auris.docx for details.
================================================================================================#=
*/
// ACCEL information other modules may need to access:
#include "lis3dsh.h"

#include <stdint.h>
#include <stdbool.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
// Does the needful to initialize the module.
// This should be called only once.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
void AccelServ_Init(void);

// =============================================================================================#=
//  Called from ISR to indicate data is ready
// =============================================================================================#=
void AccelServ_InterruptHandler(void);
