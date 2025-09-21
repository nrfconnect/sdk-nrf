#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_BOARD_NRF54L15DK_NRF54L15_CPUAPP)
  board_runner_args(nrfutil
    "--ext-mem-config-file=${CMAKE_CURRENT_SOURCE_DIR}/boards/nrf54l15dk_spi_nrfutil_config.json"
  )
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfu_extra_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Link with MCUboot bootutil library (provides boot_image_load_header API)
target_link_libraries(app PRIVATE MCUBOOT_BOOTUTIL)
