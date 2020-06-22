#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

if ((BOARD STREQUAL nrf5340pdk_nrf5340_cpuappns) OR
    (BOARD STREQUAL nrf5340pdk_nrf5340_cpuapp))
  message(FATAL_ERROR "This sample is not supported on the nRF53 application core")
endif()

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)
# NORDIC SDK APP END
zephyr_library_include_directories(.)
