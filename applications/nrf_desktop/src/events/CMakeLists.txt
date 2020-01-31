#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/button_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/click_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/selector_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/passkey_event.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_event.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/module_state_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/motion_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wheel_event.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_event.c)

target_sources_ifdef(CONFIG_DESKTOP_HIDS_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hids_event.c)
