# STM32F407 Discovery Kit Driver Code Repository

## Overview

Welcome to the STM32F407 Discovery Kit Driver Code Repository! This repository is dedicated to refining and showcasing my skills in writing driver codes for various components. The primary focus is on developing and testing driver code for the STM32F407 Discovery Kit.

## Components

### LIS3DSH Accelerometer

The first component added to this repository is the LIS3DSH accelerometer. This driver code will enable the STM32F407 Discovery Kit to interface with the LIS3DSH accelerometer, allowing for accurate motion sensing and data acquisition.

## Repository Structure

- **Src/**: Houses the main application and system-level code.
- **Inc/**: Includes header files for the main application.
- **Middleware/**: Contains ST middleware libraries.
- **Drivers/**: Contains HAL drivers and other dependencies provided by ST.
- **board/**: Contains PORT and PIN Macros used in application code for STM32F407 Mirco.
- **platform/**: Contains code for hardware interface, such as GPIO and SPI.
- **sensing/**: Includes driver code for sensing components, such as the accelerometer.

## Documentation
- [STM32F407 Discovery Kit](https://www.st.com/resource/en/user_manual/dm00039084-discovery-kit-with-stm32f407vg-mcu-stmicroelectronics.pdf)
- [LIS3DSH Accelerometer](https://www.st.com/resource/en/datasheet/lis3dsh.pdf)
- [MX25V1635F Flash](https://www.mxic.com.tw/Lists/Datasheet/Attachments/8999/MX25V1635F,%202.5V,%2016Mb,%20v1.6.pdf)
- [MB85RS256TY FRAM](https://www.fujitsu.com/uk/Images/MB85RS256TY-Industrial.pdf)

## Pinout Diagram
- **Accelerometer**

![!\[accel_pinout\]](resources/pinout/accel_pinout.PNG)

- **Flash and FRAM**

![!\[ext_flash_fram_pinout\]](resources/pinout/ext_flash_fram_pinout.PNG)

## Getting Started

To get started with the driver code in this repository, follow these steps:

1. **Clone the repository**:
   ```bash
   git clone https://gitlab.com/belina_sainju/driver-development.git
   ```
2. **Navigate to the repository directory**:
   ```bash
   cd driver-development
   ```
3. **Open the project in STM32CubeIDE**:
   - Launch STM32CubeIDE.
   - Select **File > Open Projects from File System...**.
   - Browse to the cloned repository directory and import the project.

4. **Build and run the example projects**:
   - Select the project in the Project Explorer.
   - Click on the **Build** button (hammer icon) to compile the project.
   - Click on the **Run** button (green play icon) to upload and run the project on your STM32F407 Discovery Kit.

## Coding Standard

This is the coding standard used for this project work.

## 1. Constants
- All constants should be in uppercase letters.
- Use underscores (`_`) instead of spaces.

**Example:**
```c
const int MAX_BUFFER_SIZE = 1024;
#define DEFAULT_USER "Guest";
```

## 2. Variables

### 2.a. Local Variables
- All local variables should be in camelCase format.

**Example:**
```c
int bufferSize = 512;
string userName = "JohnDoe";
```

### 2.b. Global Variables
- All private global variables should be in camelCase format with a **'x' prefix**.

**Example:**
```c
static int xBufferSize = 512;
```

- Avoid using global variables that go outside of scope of the file.

### 2.c. Arguments/Parameters
- All arguments/parameters of a function should be in lower case with a camelCase format.

**Example:**
```c
bool MX25_READ(uint32_t flashAddress, uint8_t *targetAddress, uint32_t byteLength);
```

## 3. Function Names

### 3.a. Private Functions
- All private function names should start with a **prefix, in lower case**, where the prefix is the name of the file or a word that reflects the function of the filename.

**Example:**
For file `lis3dsh.c` which is a driver file for an accelerometer, the prefix could be 'lis3dsh' or 'accel.'
```c
static void accelRead() {
    // Function implementation
}

static void lis3dshRead() {
    // Function implementation
}
```

### 3.b. Public Functions
- All public function names should start with a **prefix and an underscore, in upper case**, where the prefix is the name of the file or a word that reflects the function of the filename.

**Example:**
For file `lis3dsh.c` which is a driver file for an accelerometer, the prefix could be 'LIS3DSH.'
```cpp
void LIS3DSH_Initialize() {
    // Function implementation
}
```

For file `spi-core.c` which is a peripheral driver for SPI, the prefix could be 'SPI.'
```c
void SPI_Initialize() {
    // Function implementation
}
```

## 4. Enums and Structs
- All enum names should end with **'_t' suffix**.
- All enum elements should be in upper case with '_' instead of space.

**Example:**
```c
typedef enum {
    FLASH_OPERATION_SUCCESS,
    FLASH_OPERATION_FAILED,
} flashReturnMsg_t;
```

- All struct elements should be in CamelCase with the first letter of each word in uppercase.

**Example:**
```c
typedef struct {
    uint8_t ModeReg;
    bool ArrangeOpt;
} FlashStatus_t;
```

## 5. Includes
- All includes should be organized in the following manner.

**Example:**
```c
#pragma once // for header files only

// Project File Header Content
#include "header-file.h"

// Other Project Content/Dependencies
#include "spi-core.h"

// Third Party Libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Standard Library Headers
#include <stdlib.h>
```

## Contact

For any questions or suggestions, feel free to open an issue or contact me directly at belina.sainju@twisthink.com
