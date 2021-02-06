# Summary
The aim of the ATXFox project was to design a laboratory power supply from a standard ATX power supply, by adding some useful features such as specific connectors, voltage and current monitoring, IHM, etc...

Each voltage output of the ATX has its own front-end, formed by a PSFE and TRCS boards pair. The PSFE board is a generic power supply interface including control switches, standard connectors, LCD screen for voltage and current display, and Sigfox connectivity for optional remote monitoring.

# Hardware
The board was designed on **Circuit Maker V1.3**. Hardware documentation and design files are available @ https://workspace.circuitmaker.com/Projects/Details/Ludovic-Lesur/PSFEHW1-0

# Embedded software

## Environment
The embedded software was developed under **Eclipse IDE** version 2019-06 (4.12.0) and **GNU MCU** plugin. The `script` folder contains Eclipse run/debug configuration files and **JLink** scripts to flash the MCU.

## Target
The PSFE board is based on the **STM32L031G6U6** microcontroller of the STMicroelectronics L0 family.

## Structure
The project is organized as follow:
* `inc` and `src`: **source code** split in 4 layers:
    * `registers`: MCU **registers** adress definition.
    * `peripherals`: internal MCU **peripherals** drivers.
    * `components`: external **components** drivers.
    * `applicative`: high-level **application** layers.
* `startup`: MCU **startup** code (from ARM).
* `linker`: MCU **linker** script (from ARM).
