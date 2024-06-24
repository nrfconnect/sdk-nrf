#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(BCT)

# Include application events and configuration headers
zephyr_library_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../nrf5340_audio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../nrf5340_audio/src/bluetooth
    ${CMAKE_CURRENT_SOURCE_DIR}/../../nrf5340_audio/src/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/../../nrf5340_audio/src/utils/macros
)

zephyr_library_include_directories(app PRIVATE
    ${ZEPHYR_NRF_MODULE_DIR}/boards/arm/nrf5340_audio_dk_nrf5340)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../nrf5340_audio/src/bluetooth bluetooth_build)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../nrf5340_audio/src/modules/sd_card.c)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})
