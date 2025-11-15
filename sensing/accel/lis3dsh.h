#pragma once
/**
 *  @file                   sensing/accel/lis3dsh.h
 *  @brief                  A software module that provides communication services
 *                          for reading and writing to the LIS3DSH accelerometer
 *  @date                   Nov 1, 2024
 *  @author                 Belina Sainju
 *  @remark
 */

#ifndef LIS3DSH_H_
#define LIS3DSH_H_

/* This ifdef allows the header to be used from both C and C++. */
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

    // Structure to hold IMU X, Y, and Z accel data
    typedef struct __attribute__((packed))
    {
        int16_t accelX_mg;
        int16_t accelY_mg;
        int16_t accelZ_mg;
    } LIS3DSH_Data_t;

    // =============================================================================================#=
    // Public API Functions
    // =============================================================================================#=

    // =============================================================================================#=
    // Check if this accel module is initialized
    // =============================================================================================#=
    bool LIS3DSH_IsModuleInitialized(void);

    // =============================================================================================#=
    // Read Accel device ID
    // =============================================================================================#=
    uint8_t LIS3DSH_ReadID(void);

    // =============================================================================================#=
    // Perform Soft Reset
    // =============================================================================================#=
    bool LIS3DSH_PerformSoftReset(void);

    // =============================================================================================#=
    // Enable the data ready interrupts for the accelerometer and gyroscope on the INT1 pin
    //
    // Returns true if write to enable interrupts was successful
    // =============================================================================================#=
    bool LIS3DSH_EnableInterrupt(void);

    // =============================================================================================#=
    // Reads the accelerometer and gyroscope data from the IMU
    //
    // TODO: Return ICM42688_ImuData_t holding accel (mg) and gyro (dps) data
    // =============================================================================================#=
    bool LIS3DSH_ReadAccelData(void);

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
    // One-time startup initialization for the LIS3DSH accelerometer
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+~
    bool LIS3DSH_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* LIS3DSH_H_ */
