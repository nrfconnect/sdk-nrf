#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_state.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_ADVERTISING_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_adv.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_STATE_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_state.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_state.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

target_sources_ifdef(CONFIG_DESKTOP_POWER_MANAGER_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_manager.c)

target_sources_ifdef(CONFIG_DESKTOP_SHELL_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shell.c)
