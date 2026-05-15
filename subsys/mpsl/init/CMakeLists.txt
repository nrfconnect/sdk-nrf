#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Create the library name based on the $ZEPHYR_NRF_MODULE_DIR path.
# This removes the `nrf` part from the library name. It allows to amend library with
# zephyr_library_amend() from out-of-tree modules.
zephyr_library_get_current_dir_lib_name(${ZEPHYR_NRF_MODULE_DIR} lib_name)
zephyr_library_named(${lib_name})

zephyr_library_sources(mpsl_init.c)

if(CONFIG_SOC_COMPATIBLE_NRF52X OR
    CONFIG_SOC_COMPATIBLE_NRF53X OR
    CONFIG_SOC_COMPATIBLE_NRF54LX OR
    CONFIG_SOC_SERIES_NRF54H OR
    CONFIG_SOC_SERIES_NRF71)
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
