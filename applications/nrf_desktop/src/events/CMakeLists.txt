#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/battery_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/ble_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/hid_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/motion_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/passkey_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/selector_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/wheel_event.c
)

target_sources_ifdef(CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_dongle_peer_event.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config_event.c)

target_sources_ifdef(CONFIG_DESKTOP_CPU_MEAS_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cpu_load_event.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_event.c)

target_sources_ifdef(CONFIG_DESKTOP_HIDS_ENABLE app
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hids_event.c)
