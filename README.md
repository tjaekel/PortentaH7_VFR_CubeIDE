# PortentaH7_VFR
 Portenta H7 "Visual Flight Radar"

## STM32CubeIDE project
This project is __not__ an _Arduino/mbed sketch_. It is native C-code (not C++), as a STM32CubeIDE project. It uses the STM HAL drivers, CMSIS FreeRTOS.

## Project Purpose
- use the PDM MICs in order to track airplanes (above my home)
- Pan and Tilt the board (the MICs, with camera) to follow an airplane
- focus camera (not yet used) to track an airplane

## HW Requirements
- Portenta H7 MCU module
- Portenta H7 VisionShield: using ETH connection and PDM MICs on board
- (optional) Breakout Board: using ETH and USB-A connection

## Features of the project
- it provides a UART shell via the USB-C connection, see "help" command there
- running FreeRTOS
- Network support: ETH (not WiFi), for web server running on MCU, UDP transfer, e.g. Audio via UDP, network commands to do SPI, I2C
- using SDRAM: used for Pico-C scripts
- Pico-C: a C-code interpreter (no compiler needed, write C-code scripts)
- PDM MICs for Audio to host (as VBAN, VB-Audio Voicemeeter) - via ETH network
- two PWM channels for RC model servos
- QSPI flash: used for man pages
- SPI and I2C peripherals
- USB-A on Breakout Board (for Audio, prepared for USB as external memory device, a second VCP UART)

## Full Open Source
The project has all source code files, including the PMIC configuration, INT vectors, drivers, startup - not using any Arduino/mbed library.

## Without or with Arduino Bootloader
Project (linker script) can be configured to use an external debugger and utilize the entire Flash ROM space (overwrite the Bootloader).
Possible also to keep the Bootloader and flash MCU FW via command line.

