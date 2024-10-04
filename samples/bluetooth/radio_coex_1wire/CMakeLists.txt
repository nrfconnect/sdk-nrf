# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project("External radio Bluetooth coexistence sample")

if (NOT CONFIG_SOC_SERIES_NRF52X AND NOT CONFIG_SOC_SERIES_NRF54LX
    AND NOT CONFIG_SOC_SERIES_NRF54HX)
	message(FATAL_ERROR "This sample is only supported on the nRF52, nRF54L and nRF54H series.")
endif()

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)
# NORDIC SDK APP END
