#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE
		     ${CMAKE_CURRENT_SOURCE_DIR}/module_state_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/peer_conn_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/ble_ctrl_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/ble_data_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/cdc_data_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/uart_data_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/fs_event.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/power_event.c)
