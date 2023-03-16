#
# Copyright (c) 2018-2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE "dts.overlay")

if(BOARD STREQUAL bl5340pa_dvk_cpuapp)
    set(hci_rpmsg_OVERLAY_CONFIG ${CMAKE_SOURCE_DIR}/child_image/hci_rpmsg.bl5340pa.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app PRIVATE
	${app_sources}
)
# NORDIC SDK APP END

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
