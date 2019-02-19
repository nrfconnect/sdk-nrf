#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

# Set supported board
set(NRF_SUPPORTED_BOARDS
  nrf52_pca10040
  nrf52840_pca10056
  )

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}.overlay")
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}.overlay")
endif()

include(../../../cmake/boilerplate.cmake)

# Define configuration file
macro(set_conf_file)
  set(CONF_FILE
	"configuration/prj.conf"
	)
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app  PRIVATE ${app_sources})

# Include configuration headers
zephyr_library_include_directories(configuration)
