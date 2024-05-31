#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(matter-light-switch)

# Enable GNU STD support and initialize the Matter Data Model.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/src/app/chip_data_model.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

target_include_directories(app PRIVATE
    src
)

target_compile_options(app PRIVATE -Wno-deprecated-declarations)

target_sources(app PRIVATE
    src/app_task.cpp
    src/light_switch.cpp
    src/main.cpp
    src/shell_commands.cpp
    ${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/src/binding/binding_handler.cpp
)

chip_configure_data_model(app
    INCLUDE_SERVER
    BYPASS_IDL
    GEN_DIR src/zap-generated
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/light_switch.zap
)
# NORDIC SDK APP END
