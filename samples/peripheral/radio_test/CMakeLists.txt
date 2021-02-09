#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

option(NRF21540EK_FEM "Enable support for the nRF21540-EK FEM." OFF)

if(NRF21540EK_FEM)
  set(DTC_OVERLAY_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/configuration/dts-nrf21540-fem.overlay"
    CACHE STRING "" FORCE)

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}.overlay")
    set(DTC_OVERLAY_FILE "${DTC_OVERLAY_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}.overlay"
        CACHE STRING "" FORCE)
  endif()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# Application sources
target_sources_ifdef(CONFIG_NRF21540_FEM app
	PRIVATE ../../bluetooth/direct_test_mode/src/fem/nrf21540.c)
zephyr_include_directories(../../bluetooth/direct_test_mode/src/fem/)

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app PRIVATE
	${app_sources}
)
# NORDIC SDK APP END
