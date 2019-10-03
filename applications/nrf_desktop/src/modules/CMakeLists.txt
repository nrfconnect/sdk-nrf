#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_state.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_bond.c)

target_sources_ifdef(CONFIG_BT_PERIPHERAL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_latency.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_ADVERTISING_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_adv.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_DISCOVERY_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_discovery.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_SCANNING_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_scan.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_QOS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_qos.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_SCANNING_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_forward.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_STATE_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_state.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_state.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_STREAM_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_stream.c)

target_sources_ifdef(CONFIG_DESKTOP_CPI_STATE_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cpi_state.c)

target_sources_ifdef(CONFIG_DESKTOP_POWER_MANAGER_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/power_manager.c)

target_sources_ifdef(CONFIG_DESKTOP_SHELL_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/shell.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu.c)

target_sources_ifdef(CONFIG_DESKTOP_FN_KEYS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fn_keys.c)

target_sources_ifdef(CONFIG_DESKTOP_WATCHDOG_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/watchdog.c)

target_sources_ifdef(CONFIG_DESKTOP_BAS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bas.c)

target_sources_ifdef(CONFIG_BT_PERIPHERAL
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dev_descr.c)

target_sources_ifdef(CONFIG_DESKTOP_HIDS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hids.c)

target_sources_ifdef(CONFIG_DESKTOP_HFCLK_LOCK_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hfclk_lock.c)

target_sources_ifdef(CONFIG_DESKTOP_CONSTLAT_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/constlat.c)
