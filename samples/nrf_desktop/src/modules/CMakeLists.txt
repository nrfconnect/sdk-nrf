#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_adv.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_state.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_state.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_manager.c)
