#pragma once

/**
 *  @file                   sensing/accel/lis3dsh-registers.h
 *  @brief                  A software module that provides registers for communicating to the LIS3DSH.
 *  @date                   Nov 1, 2024
 *  @author                 Belina Sainju
 *  @remark
 */

#include <stdint.h>

#define LIS3DSH_WHO_AM_I_REGISTER_ADDR 0x0F
#define LIS3DSH_OUT_X_L_REGISTER_ADDR 0x28
#define LIS3DSH_OUT_X_H_REGISTER_ADDR 0x29
#define LIS3DSH_OUT_Y_L_REGISTER_ADDR 0x2A
#define LIS3DSH_OUT_Y_H_REGISTER_ADDR 0x2B
#define LIS3DSH_OUT_Z_L_REGISTER_ADDR 0x2C
#define LIS3DSH_OUT_Z_H_REGISTER_ADDR 0x2D
#define LIS3DSH_CTRL_REG3_REGISTER_ADDR 0x23
#define LIS3DSH_CTRL_REG4_REGISTER_ADDR 0x20
#define LIS3DSH_CTRL_REG5_REGISTER_ADDR 0x24

/* Register Types ---------------------------------------------------------*/

// CTRL_REG3
typedef struct
{
    uint8_t STRT : 1;
    uint8_t Reserved : 1;
    uint8_t VFILT : 1;
    uint8_t INT1_EN : 1;
    uint8_t INT2_EN : 1;
    uint8_t IEL : 1;
    uint8_t IEA : 1;
    uint8_t DR_EN : 1;
} LIS3DSH_CtrlReg3_t;

// CTRL_REG4
typedef struct
{
    uint8_t Xen : 1;
    uint8_t Yen : 1;
    uint8_t Zen : 1;
    uint8_t BDU : 1;
    uint8_t ODR : 4;
} LIS3DSH_CtrlReg4_t;

// CTRL_REG5
typedef struct
{
    uint8_t SIM : 1;
    uint8_t ST : 2;
    uint8_t FSCALE : 3;
    uint8_t BW : 2;
} LIS3DSH_CtrlReg5_t;
