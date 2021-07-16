#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pelion.cpp)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pelion_fcc.cpp)

target_sources_ifdef(CONFIG_PELION_CLIENT_OMA_STOPWATCH
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/oma_stopwatch.cpp)

target_sources_ifdef(CONFIG_PELION_CLIENT_OMA_DIGITAL_INPUT
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/oma_digital_input.cpp)

target_sources_ifdef(CONFIG_BOOTLOADER_MCUBOOT
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu.c)

target_sources_ifdef(CONFIG_CAF_LEDS
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

target_sources_ifdef(CONFIG_PELION_DEBUG_TRACE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pelion_trace.c)
