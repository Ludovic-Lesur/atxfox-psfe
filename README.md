# Description

The aim of the **ATXFox project** was to design a laboratory power supply from a standard ATX power supply, by adding some useful features such as specific connectors, voltage and current monitoring, IHM, etc...

Each voltage output of the ATX has its own front-end, formed by a **PSFE** and **TRCS** boards pair. The PSFE board is a generic power supply interface including control switches, standard connectors, LCD screen for voltage and current display, and Sigfox connectivity for optional remote monitoring. The TRCS is dedicated to current sensing with three ranges, driven by the PSFE board.

# Hardware

The boards were designed on **Circuit Maker V1.3**. Below is the list of hardware revisions:

| Hardware revision | Description | Status |
|:---:|:---:|:---:|
| [PSFE HW1.0](https://365.altium.com/files/C6DA5B00-C92D-11EB-A2F6-0A0ABF5AFC1B) | Initial version. | :white_check_mark: |
| [TRCS HW1.0](https://365.altium.com/files/C4EB9CA3-C92D-11EB-A2F6-0A0ABF5AFC1B) | Initial version. | :white_check_mark: |

# Embedded software

## Environment

The embedded software is developed under **Eclipse IDE** version 2023-09 (4.29.0) and **GNU MCU** plugin. The `script` folder contains Eclipse run/debug configuration files and **JLink** scripts to flash the MCU.

## Target

The PSFE board is based on the **STM32L031G6U6** microcontroller of the STMicroelectronics L0 family. Each hardware revision has a corresponding **build configuration** in the Eclipse project, which sets up the code for the selected board version.

## Structure

The project is organized as follow:

* `inc` and `src`: **source code** split in 5 layers:
    * `registers`: MCU **registers** adress definition.
    * `peripherals`: internal MCU **peripherals** drivers.
    * `utils`: **utility** functions.
    * `components`: external **components** drivers.
    * `applicative`: high-level **application** layers.
* `startup`: MCU **startup** code (from ARM).
* `linker`: MCU **linker** script (from ARM).
