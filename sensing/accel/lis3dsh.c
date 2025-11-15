/*
 * lis3dsh.c
 *
 *  Created on: Nov 1, 2024
 *      Author: Belina Sainju
 */

#include "lis3dsh.h"
#include "lis3dsh-registers.h"

#include "../../platform/gpio/gpio.h"
#include "board-model.h"
#include "spi/spi-core.h"

#include "stm32f4xx_hal_gpio.h"

#include <stdio.h>

// Accel XYZ Data Indices
#define ACCEL_X_LSB 0
#define ACCEL_X_MSB 1
#define ACCEL_Y_LSB 2
#define ACCEL_Y_MSB 3
#define ACCEL_Z_LSB 4
#define ACCEL_Z_MSB 5
#define ACCEL_DATA_NUM_BYTES 6

#define LIS3DSH_ACCEL_SENSITIVITY_MG 0.06 // For +/- 2g

// LIS3DSH Device ID
#define LIS3DSH_DEVICE_ID 0x3F

#define LIS3DSH_RESET_TIME_MS 3000

// Accel register and address length
#define LIS3DSH_ADDRESS_LEN 1
#define LIS3DSH_REGISTER_LEN 1

// READ Bitmask
#define LIS3DSH_READ_BITMASK 0x80

// Available output data rates for LIS3DSH
typedef enum
{
	LIS3DSH_SAMPLE_RATE_OFF = 0, // Power down
	LIS3DSH_SAMPLE_RATE_3_125HZ,
	LIS3DSH_SAMPLE_RATE_6_25HZ,
	LIS3DSH_SAMPLE_RATE_12_5HZ,
	LIS3DSH_SAMPLE_RATE_25HZ,
	LIS3DSH_SAMPLE_RATE_50HZ,
	LIS3DSH_SAMPLE_RATE_100HZ,
	LIS3DSH_SAMPLE_RATE_400HZ,
	LIS3DSH_SAMPLE_RATE_800HZ,
	LIS3DSH_SAMPLE_RATE_1600HZ,
	LIS3DSH_SAMPLE_RATE_NUMOF
} LIS3DSH_AccelOutputDataRate_t;

// Available anti-aliasing filter bandwidth rates for LIS3DSH
typedef enum
{
	LIS3DSH_ANTI_ALIAS_FILTER_BW_800HZ = 0, // Default
	LIS3DSH_ANTI_ALIAS_FILTER_BW_200HZ,
	LIS3DSH_ANTI_ALIAS_FILTER_BW_400HZ,
	LIS3DSH_ANTI_ALIAS_FILTER_BW_50HZ,
	LIS3DSH_ANTI_ALIAS_FILTER_BW_NUMOF
} LIS3DSH_AntiAliasFilterBW_t;

static bool xModuleInitialized = false;

/*** Private Functions ***/

// Helper function to set CS pin high
static void accelChipSelectHigh(void)
{
	HAL_GPIO_WritePin(PORT(ACCEL_CS), PIN(ACCEL_CS), GPIO_PIN_SET);
}

// Helper function to set CS pin low
static void accelChipSelectLow(void)
{
	HAL_GPIO_WritePin(PORT(ACCEL_CS), PIN(ACCEL_CS), GPIO_PIN_RESET);
}

/*
 * Function:       Read ACCEL registers
 * Arguments:      regToRead, dataReceived, lengthToReceive
 * Description:    Reads ACCEL register and stores read data in dataReceived
 * Return Message: Bool
 */
static bool accelRead(uint8_t *regToRead, uint8_t *dataReceived, uint8_t lengthToReceive)
{
	bool status = false;

	uint8_t readCommand = (uint8_t)*regToRead | LIS3DSH_READ_BITMASK;

	accelChipSelectLow();
	status = SPI_Transfer(LIS3DSH_ACCEL, (uint8_t *)&readCommand, LIS3DSH_REGISTER_LEN, dataReceived,
						  lengthToReceive);
	accelChipSelectHigh();

	return status;
}

/*
 * Function:       Write to ACCEL registers
 * Arguments:      regToWrite, dataToWrite, dataLength
 * Description:    Writes data to ACCEL register
 * Return Message: Bool
 */
static bool accelWrite(uint8_t *regToWrite, uint8_t *dataToWrite,
					   uint8_t dataLength)
{
	bool status = false;

	accelChipSelectLow();
	status = SPI_Transfer(LIS3DSH_ACCEL, regToWrite, LIS3DSH_REGISTER_LEN, NULL, 0);

	if (status)
	{
		status = SPI_Transfer(LIS3DSH_ACCEL, dataToWrite, dataLength, NULL, 0);
	}
	accelChipSelectHigh();

	return status;
}

/*
 * Function:       Convert raw accel data to mg
 * Arguments:      rawAccelData
 * Description:    Helper function to convert raw accel data to mg
 * Return Message: int16_t
 */
static int16_t accelConvertDataToMg(int16_t rawAccelData)
{
	printf("\nRaw Accel: %d\n", rawAccelData);
	return (rawAccelData * LIS3DSH_ACCEL_SENSITIVITY_MG);
}

// Temporary helper function to read a register
static bool accelReadRegister(uint8_t reg)
{
	bool status = false;
	uint8_t data = 0;

	status = accelRead(&reg, &data, LIS3DSH_REGISTER_LEN);
	if (status)
	{
		printf("Register %02X Data %d", reg, data);
	}
	else
	{
		printf("Failed to read register %02X", reg);
	}
	return status;
}

/*
 * Function:       Configure LIS3DSH module
 * Arguments:      void
 * Description:    Configures registers to set output data rate, anti aliasing filter bandwidth
 * Return Message: bool
 */
static bool accelConfigure(LIS3DSH_AccelOutputDataRate_t outputDataRate, LIS3DSH_AntiAliasFilterBW_t antiAliasFilterBW)
{
	bool status = false;

	uint8_t reg = LIS3DSH_CTRL_REG4_REGISTER_ADDR;
	LIS3DSH_CtrlReg4_t ctrlReg4data = {0};

	status = accelRead(&reg, (uint8_t *)&ctrlReg4data, LIS3DSH_REGISTER_LEN);

	if (status)
	{
		ctrlReg4data.ODR = outputDataRate; // 800 Hz
		ctrlReg4data.BDU = 1;			   // Update output registers only when both MSB and LSB have been read
		// XYZ output is enabled by default

		status = accelWrite((uint8_t *)&reg, (uint8_t *)&ctrlReg4data, LIS3DSH_REGISTER_LEN);

		if (!status)
		{
			printf("Failed to write to CTRL REG4\n");
			return status;
		}
	}
	else
	{
		printf("Failed to read CTRL REG4\n");
		return status;
	}

	reg = LIS3DSH_CTRL_REG5_REGISTER_ADDR;
	LIS3DSH_CtrlReg5_t ctrlReg5data = {0};

	status = accelRead(&reg, (uint8_t *)&ctrlReg5data, LIS3DSH_REGISTER_LEN);

	if (status)
	{
		ctrlReg5data.BW = antiAliasFilterBW; // Anti aliasing filter bandwidth 200 Hz

		status = accelWrite((uint8_t *)&reg, (uint8_t *)&ctrlReg5data, LIS3DSH_REGISTER_LEN);

		if (!status)
		{
			printf("Failed to write to CTRL REG5\n");
			return status;
		}
	}
	else
	{
		printf("Failed to read CTRL REG5\n");
		return status;
	}

	return status;
}

/*** Public Functions ***/

/*
 * Function:       Returns if LIS3DSH module is initialized
 * Arguments:      void
 * Description:    Checks for the global variable xModuleInitialized
 * Return Message: bool
 */
bool LIS3DSH_IsModuleInitialized(void)
{
	if (!xModuleInitialized)
	{
		printf("LIS3DSH: Module Not Initialized");
		return false;
	}

	return true;
}

/*
 * Function:       Perform soft reset
 * Arguments:      void
 * Description:    Write to CTRL Reg3 to perform soft reset
 * Return Message: bool
 */
bool LIS3DSH_PerformSoftReset(void)
{
	bool status = false;
	uint8_t reg = LIS3DSH_CTRL_REG3_REGISTER_ADDR;
	LIS3DSH_CtrlReg3_t data = {0};

	status = accelRead(&reg, (uint8_t *)&data, LIS3DSH_REGISTER_LEN);

	if (status)
	{
		data.STRT = 1; // Perform soft reset

		status = accelWrite((uint8_t *)&reg, (uint8_t *)&data, LIS3DSH_REGISTER_LEN);

		if (status)
		{
			// Wait for soft reset
			HAL_Delay(LIS3DSH_RESET_TIME_MS);
		}
		else
		{
			printf("Failed to enable soft reset\n");
		}
	}

	return status;
}

/*
 * Function:       Read Accel Output data
 * Arguments:      void
 * Description:    Reads XYZ Output registers, converts raw data to mg
 * Return Message: bool
 */
bool LIS3DSH_ReadAccelData(void)
{
	LIS3DSH_Data_t accelData = {0};

	uint8_t data[ACCEL_DATA_NUM_BYTES];

	uint8_t reg1 = LIS3DSH_OUT_X_L_REGISTER_ADDR;

	bool status = false;

	status = accelRead(&reg1, data, ACCEL_DATA_NUM_BYTES);

	if (status)
	{
		accelData.accelX_mg = accelConvertDataToMg((int16_t)data[ACCEL_X_LSB] | ((int16_t)data[ACCEL_X_MSB] << 8));

		accelData.accelY_mg = accelConvertDataToMg((int16_t)data[ACCEL_Y_LSB] | ((int16_t)data[ACCEL_Y_MSB] << 8));

		accelData.accelZ_mg = accelConvertDataToMg((int16_t)data[ACCEL_Z_LSB] | ((int16_t)data[ACCEL_Z_MSB] << 8));
	}
	else
	{
		// Reset accel data if failed to read new accel data
		accelData.accelX_mg = 0;
		accelData.accelY_mg = 0;
		accelData.accelZ_mg = 0;

		printf("Failed to read ACCEL data\n");
	}

	printf("\r\nAccel X: %d\n", accelData.accelX_mg);
	printf("Accel Y: %d\n", accelData.accelY_mg);
	printf("Accel Z: %d\n", accelData.accelZ_mg);

	return status;
}

/*
 * Function:       Enable Interrupt
 * Arguments:      void
 * Description:    Enable Interrupt
 * Return Message: bool
 */
bool LIS3DSH_EnableInterrupt(void)
{
	bool status = false;
	uint8_t reg = LIS3DSH_CTRL_REG3_REGISTER_ADDR;
	LIS3DSH_CtrlReg3_t data = {0};

	status = accelRead(&reg, (uint8_t *)&data, LIS3DSH_REGISTER_LEN);

	if (status)
	{
		data.IEL = 1;	  // Set INT1 pulsed mode
		data.IEA = 1;	  // Set INT1 active high
		data.DR_EN = 1;	  // Map DataReady to INT1
		data.INT1_EN = 1; // Enable INT1

		status = accelWrite((uint8_t *)&reg, (uint8_t *)&data, LIS3DSH_REGISTER_LEN);

		if (status)
		{
			HAL_NVIC_EnableIRQ(EXTI0_IRQn);
			printf("Enabled INT1\n");
		}
		else
		{
			printf("Failed to enable INT1\n");
		}
	}
	return status;
}

/*
 * Function:       Read WHO_AM_I register
 * Arguments:      None
 * Description:    Reads data from WHO_AM_I register
 * Return Message: Bool
 */
uint8_t LIS3DSH_ReadID(void)
{
	uint8_t id = 0;
	uint8_t reg = LIS3DSH_WHO_AM_I_REGISTER_ADDR;
	bool status = false;

	// Read WHO_AM_I register
	status = accelRead(&reg, &id, LIS3DSH_REGISTER_LEN);

	if (status)
	{
		// Verify ID is correct
		if (id == LIS3DSH_DEVICE_ID)
		{
			printf("Accel WHO_AM_I: %d\n", LIS3DSH_DEVICE_ID);
			return id;
		}
		else
		{
			return 0;
		}
	}
	return 0;
}

/*
 * Function:       LIS3DSH_Init
 * Arguments:      None
 * Description:    Initialize Chip Select and Interrupt pin for LIS3DSH accel,
 *                  Set Chip Select pin high
 * Return Message: true
 */
bool LIS3DSH_Init(void)
{
	bool status = false;
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// Init ACCEL INT pin
	GPIO_ClockEnable(PORT(ACCEL_INT1));
	GPIO_InitStruct.Pin = PIN(ACCEL_INT1);
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(PORT(ACCEL_INT1), &GPIO_InitStruct);

	// Set INT priority
	HAL_NVIC_SetPriority(EXTI0_IRQn, 15, 15);

	// Enable INT
	HAL_NVIC_EnableIRQ(EXTI0_IRQn);

	// Init ACCEL CS pin
	GPIO_ClockEnable(PORT(ACCEL_CS));
	GPIO_InitStruct.Pin = PIN(ACCEL_CS);
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(PORT(ACCEL_CS), &GPIO_InitStruct);

	// Set CS pin high (Set it low during SPI comm)
	HAL_GPIO_WritePin(PORT(ACCEL_CS), PIN(ACCEL_CS), GPIO_PIN_SET);

	// Verify SPI comms work by reading WhoAmI register (0x0f)
	status = LIS3DSH_ReadID();
	//	if (status)
	//	{
	//		status = LIS3DSH_PerformSoftReset();
	//	}
	if (status)
	{
		status = accelConfigure(LIS3DSH_SAMPLE_RATE_800HZ, LIS3DSH_ANTI_ALIAS_FILTER_BW_200HZ);
		xModuleInitialized = true;
	}
	if (status)
	{
		status = LIS3DSH_EnableInterrupt();
	}

	if (status)
	{
		printf("Accel Init Complete\n");
	}
	else
	{
		printf("Accel Init Failed\n");
	}

	return status;
}
