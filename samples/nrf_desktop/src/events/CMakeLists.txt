#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/button_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/module_state_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/motion_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wheel_event.c)
