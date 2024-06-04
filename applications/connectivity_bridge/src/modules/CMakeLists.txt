#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_PM_DEVICE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_handler.c)

target_sources_ifdef(CONFIG_SERIAL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/uart_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_CDC_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_cdc_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_BLE_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_MSC_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fs_handler.c)

target_sources_ifdef(CONFIG_BRIDGE_CMSIS_DAP_BULK_ENABLE
		     app PRIVATE
		     ${CMAKE_CURRENT_SOURCE_DIR}/usb_bulk_commands.c
		     ${CMAKE_CURRENT_SOURCE_DIR}/usb_bulk_interface.c)
