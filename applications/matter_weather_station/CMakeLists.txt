#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(matter-weather-station)

# Enable GNU STD support.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

# Include Data Model utils
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/data_model.cmake)

target_include_directories(app PRIVATE
    src
    ${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}
)

target_sources(app PRIVATE
    src/app_task.cpp
    src/main.cpp
    src/battery.cpp
    src/buzzer.cpp
)

ncs_configure_data_model(
    ZAP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILES_PATH}/${CONFIG_NCS_SAMPLE_MATTER_ZAP_FILE_NAME}.zap
)
# NORDIC SDK APP END
