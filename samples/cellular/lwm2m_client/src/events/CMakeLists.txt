#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gnss_pvt_event.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui_input_event.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/location_events.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/location_event_handler.c)
