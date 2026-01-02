#
# CMakeLists.txt
#
#  Created on: 02 jan. 2026
#      Author: Ludo
#

# Minimum CMake version.
cmake_minimum_required(VERSION 3.23)

# Project creation.
project(atxfox-psfe)
add_executable(${PROJECT_NAME})

# Use object build mode in all sub-directories.
set(BUILD_MODE "OBJECT")

# Macro to convert options to variables.
macro(add_compilation_flag FLAG_NAME FLAG_DESCRIPTION FLAG_DEFAULT_VALUE)
    option(FLAG_NAME FLAG_DESCRIPTION FLAG_DEFAULT_VALUE)
    set(${FLAG_NAME} ${FLAG_DEFAULT_VALUE} CACHE STRING ${FLAG_DESCRIPTION})
    list(APPEND COMPILATION_FLAGS_LIST ${FLAG_NAME})
endmacro()

# Hardware compilation flags.
set(PSFE_HW_VERSION "NONE" CACHE STRING "Hardware version")

# Software compilation flags.
add_compilation_flag(PSFE_SERIAL_MONITORING "Enable serial monitoring." OFF)
add_compilation_flag(PSFE_SIGFOX_MONITORING "Enable Sigfox monitoring." OFF)

# Hardware specific settings.
# PSFE HW1.0.
if(PSFE_HW_VERSION STREQUAL "HW1_0")   
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m0plus -O1")
    set(SPSWS_MCU "stm32l0xx")
    set(PROJECT_LINKER_SCRIPT "stm32l031x6xx.ld")
# Unknown version.
else()
    message(FATAL_ERROR "Invalid hardware version.")
endif()
set(PROJECT_LINKER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/drivers/device/${SPSWS_MCU}-device/linker")

# Add hardware compilation flags.
add_compile_definitions(
    __PSFE_FLAGS_H__
    ${PSFE_HW_VERSION}
)

# Add software compilation flags.
foreach(FLAG ${COMPILATION_FLAGS_LIST})
    # Macros only defined.
    if(${FLAG} STREQUAL ON)
        add_compile_definitions(${FLAG})
    endif()
    # Macros with value.
    if((NOT ${FLAG} STREQUAL OFF) AND (NOT ${FLAG} STREQUAL ON))
        set(${FLAG} "(${${FLAG}})")
        add_compile_definitions(${FLAG}=${${FLAG}}) 
    endif()
endforeach()

# Add project sources files.
target_sources(${PROJECT_NAME}
    PRIVATE
        drivers/peripherals/src/mcu_mapping.c
        drivers/components/src/st7066u_hw.c
        drivers/components/src/td1208_hw.c
        drivers/components/src/trcs_hw.c
        drivers/utils/src/terminal_hw.c
        middleware/analog/src/analog.c
        middleware/hmi/src/hmi.c
        middleware/serial/src/serial.c
        middleware/sigfox/src/sigfox.c
        application/src/main.c
)

# Project include folders.
include_directories(${PROJECT_NAME}
    PRIVATE
        drivers/device/inc
        drivers/registers/inc
        drivers/registers/${SPSWS_MCU}-registers/inc
        drivers/peripherals/inc
        drivers/peripherals/${SPSWS_MCU}-drivers/inc
        drivers/utils/inc
        drivers/utils/embedded-utils/inc
        drivers/components/inc
        drivers/components/st7066u-driver/inc
        drivers/components/td1208-driver/inc
        drivers/components/trcs-driver/inc
        middleware/analog/inc
        middleware/hmi/inc
        middleware/serial/inc
        middleware/sigfox/inc
        application/inc
)

# Add submodules sources files.
add_subdirectory(drivers/device/${SPSWS_MCU}-device EXCLUDE_FROM_ALL)
add_subdirectory(drivers/peripherals/${SPSWS_MCU}-drivers EXCLUDE_FROM_ALL)
add_subdirectory(drivers/utils/embedded-utils EXCLUDE_FROM_ALL)
add_subdirectory(drivers/components/st7066u-driver EXCLUDE_FROM_ALL)
add_subdirectory(drivers/components/td1208-driver EXCLUDE_FROM_ALL)
add_subdirectory(drivers/components/trcs-driver EXCLUDE_FROM_ALL)

# Link libraries.
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${SPSWS_MCU}-device
        ${SPSWS_MCU}-drivers
        embedded-utils
        st7066u-driver
        td1208-driver
        trcs-driver
        c_nano
        nosys
        gcc
)

# Linker and artifact.
include(script/cmake-arm-none-eabi/linker.cmake)
include(script/cmake-arm-none-eabi/artifact.cmake)
