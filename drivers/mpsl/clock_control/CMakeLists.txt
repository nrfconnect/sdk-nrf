#
# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(nrfx_clock_mpsl.c)

target_include_directories(zephyr_interface BEFORE INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})
