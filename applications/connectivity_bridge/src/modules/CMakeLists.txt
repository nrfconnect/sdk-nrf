#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources_ifdef(CONFIG_DEVICE_POWER_MANAGEMENT
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_handler.c)

target_sources_ifdef(CONFIG_SERIAL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/uart_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_CDC_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_cdc_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_BLE_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_MSC_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fs_handler.c)
