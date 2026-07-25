# Description

The aim of the **ATXFox project** was to design a laboratory power supply from a standard ATX power supply, by adding some useful features such as specific connectors, voltage and current monitoring, IHM, etc...

Each voltage output of the ATX has its own front-end, formed by a **PSFE** and **TRCS** boards pair. The PSFE board is a generic power supply interface including control switches, standard connectors, LCD screen for voltage and current display, and Sigfox connectivity for optional remote monitoring. The TRCS is dedicated to current sensing with three ranges, driven by the PSFE board.

# Hardware

The boards were designed on **Circuit Maker V1.3**. Below is the list of hardware revisions:

| Hardware revision | Description | `cmake_hw_version` | Status |
|:---:|:---:|:---:|:---:|
| [PSFE HW1.0](https://365.altium.com/files/C6DA5B00-C92D-11EB-A2F6-0A0ABF5AFC1B) | Initial version. | `HW1_0` | :white_check_mark: |
| [TRCS HW1.0](https://365.altium.com/files/C4EB9CA3-C92D-11EB-A2F6-0A0ABF5AFC1B) | Initial version. | | :white_check_mark: |

# Embedded software

## Environment

The firmware is developed under **Eclipse IDE** and **GNU MCU** plugin. The `script` folder contains Eclipse run/debug configuration files and **JLink** scripts to flash the MCU.

## Target

The PSFE board is based on the **STM32L031G6U6** microcontroller of the STMicroelectronics L0 family. Each hardware revision has a corresponding **build configuration** in the Eclipse project, which sets up the code for the selected board version.

## Structure

The project is organized as follow:

* `drivers` :
    * `device` : MCU **startup** code and **linker** script.
    * `registers` : MCU **registers** address definition.
    * `peripherals` : internal MCU **peripherals** drivers.
    * `components` : external **components** drivers.
    * `utils` : **utility** functions.
* `middleware` :
    * `analog` : High level **analog measurements** driver.
    * `hmi` : **HMI** driver.
    * `serial` : **Serial monitoring** driver.
    * `sigfox` : **Sigfox monitoring** driver.
* `application` : Main **application**.

## Build

The project can be compiled by command line with `cmake`.

```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="script/cmake-arm-none-eabi/toolchain.cmake" \
      -DTOOLCHAIN_PATH="<arm_none_eabi_gcc_path>" \
      -DPSFE_HW_VERSION="<cmake_hw_version>" \
      -DPSFE_SERIAL_MONITORING=OFF \
      -DPSFE_SIGFOX_MONITORING=OFF \
      -G "Unix Makefiles" ..
make all
```

## Flash

### Preparation

* **Build** the desired version (with IDE or `cmake`) or **download** a specific [firmware release](https://github.com/Ludovic-Lesur/atxfox-psfe/releases) (expand the `Assets` menu, download the corresponding artifact and extract the binary files from the `zip`).
* Connect the flashing tool to the **P22 connector** located above the LCD screen.

### ST-Link on Nucleo board

* Make sure that the ST-LINK/NUCLEO jumpers (generally designated by **CN2**) are not fitted, in order to **select the external programming connector** instead of the internal MCU.
* An **MSC disk** named `NODE_XXXXXX` should be mounted by the system after USB plugging. If not, download the [ST Cube Programmer](https://www.st.com/en/development-tools/stm32cubeprog.html) software which will install the required drivers. If the MSC disk is still not mounted, follow the ST-Link probe procedure thereafter.
* **Copy/paste** or **click/drop** the `bin` file into the disk.

### ST-Link probe

* Download the [ST Cube Programmer](https://www.st.com/en/development-tools/stm32cubeprog.html) software.
* Launch the software (it might be necessary to run it as **root** or to install specific **USB rules** for the probe to be recognized).
* In the right panel, select `ST-LINK` and click `Connect`.
* Click on the `Open file` tab and select the `hex` file to flash.
* Click on the `Download` button.
* Perform a **memory check** with the `Verify` button located under the `Download` button menu.
* If the operation completed successfully, click on `Disconnect` in the right panel.

### Segger J-Link probe

* Download the [Segger J-Link](https://www.segger.com/downloads/jlink/) software.
* Launch the `JFlashLite` tool.
* Set target device to **STM32L031G6**, target interface to **SWD**, speed to **4000kHz** and click `OK`.
* Open the `hex` file to flash.
* Click on the `Program Device` button.

### Final steps

* Check on the LCD screen if the board has properly rebooted with the **expected firmware version**.
