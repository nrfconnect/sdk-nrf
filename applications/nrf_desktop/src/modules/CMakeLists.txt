#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_DESKTOP_BLE_BOND_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_bond.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_ENABLE_PASSKEY
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_passkey.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_LATENCY_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_latency.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_CONN_PARAMS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_conn_params.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_DISCOVERY_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_discovery.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_SCAN_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_scan.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_QOS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_qos.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/info.c)

target_sources_ifdef(CONFIG_DESKTOP_FAST_PAIR
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fast_pair.c)

target_sources_ifdef(CONFIG_DESKTOP_SWIFT_PAIR
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/swift_pair.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_FORWARD_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_forward.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_STATE_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_state.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_STATE_PM_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hid_state_pm.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_state.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_PM_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/usb_state_pm.c)

target_sources_ifdef(CONFIG_CAF_LEDS
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_state.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_STREAM_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_stream.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dfu.c)

target_sources_ifdef(CONFIG_DESKTOP_FN_KEYS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fn_keys.c)

target_sources_ifdef(CONFIG_DESKTOP_WATCHDOG_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/watchdog.c)

target_sources_ifdef(CONFIG_DESKTOP_BAS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bas.c)

target_sources_ifdef(CONFIG_DESKTOP_QOS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qos.c)

target_sources_ifdef(CONFIG_DESKTOP_DEV_DESCR_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dev_descr.c)

target_sources_ifdef(CONFIG_DESKTOP_HIDS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hids.c)

target_sources_ifdef(CONFIG_DESKTOP_HFCLK_LOCK_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hfclk_lock.c)

target_sources_ifdef(CONFIG_DESKTOP_CONSTLAT_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/constlat.c)

target_sources_ifdef(CONFIG_DESKTOP_FAILSAFE_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/failsafe.c)

target_sources_ifdef(CONFIG_DESKTOP_CPU_MEAS_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cpu_meas.c)

target_sources_ifdef(CONFIG_DESKTOP_NRF_PROFILER_SYNC_GPIO_ENABLE
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nrf_profiler_sync.c)
