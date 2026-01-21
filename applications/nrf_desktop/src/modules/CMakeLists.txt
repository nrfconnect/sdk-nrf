#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources_ifdef(CONFIG_DESKTOP_BLE_ADV_CTRL_ENABLE app PRIVATE ble_adv_ctrl.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_BOND_ENABLE app PRIVATE ble_bond.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_ENABLE_PASSKEY app PRIVATE ble_passkey.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_LATENCY_ENABLE app PRIVATE ble_latency.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_CONN_PARAMS_ENABLE app PRIVATE ble_conn_params.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_DISCOVERY_ENABLE app PRIVATE ble_discovery.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_SCAN_ENABLE app PRIVATE ble_scan.c)

target_sources_ifdef(CONFIG_DESKTOP_BLE_QOS_ENABLE app PRIVATE ble_qos.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE app PRIVATE info.c)

target_sources_ifdef(CONFIG_DESKTOP_FACTORY_RESET app PRIVATE factory_reset.c)

target_sources_ifdef(CONFIG_DESKTOP_FAST_PAIR app PRIVATE fast_pair.c)

target_sources_ifdef(CONFIG_DESKTOP_SWIFT_PAIR app PRIVATE swift_pair.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_FORWARD_ENABLE app PRIVATE hid_forward.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_STATE_ENABLE app PRIVATE hid_state.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_REPORT_PROVIDER_CONSUMER_CTRL app PRIVATE
  hid_provider_consumer_ctrl.c
)

target_sources_ifdef(CONFIG_DESKTOP_HID_REPORT_PROVIDER_MOUSE app PRIVATE hid_provider_mouse.c)

target_sources_ifdef(CONFIG_DESKTOP_HID_REPORT_PROVIDER_KEYBOARD app PRIVATE
  hid_provider_keyboard.c
)

target_sources_ifdef(CONFIG_DESKTOP_HID_REPORT_PROVIDER_SYSTEM_CTRL app PRIVATE
  hid_provider_system_ctrl.c
)

target_sources_ifdef(CONFIG_DESKTOP_HID_STATE_PM_ENABLE app PRIVATE hid_state_pm.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_ENABLE app PRIVATE usb_state.c)

target_sources_ifdef(CONFIG_DESKTOP_USB_PM_ENABLE app PRIVATE usb_state_pm.c)

target_sources_ifdef(CONFIG_CAF_LEDS app PRIVATE led_state.c)

target_sources_ifdef(CONFIG_DESKTOP_LED_STREAM_ENABLE app PRIVATE led_stream.c)

target_sources_ifdef(CONFIG_DESKTOP_CONFIG_CHANNEL_DFU_ENABLE app PRIVATE dfu.c)

target_sources_ifdef(CONFIG_DESKTOP_DFU_MCUMGR_ENABLE app PRIVATE dfu_mcumgr.c)

target_sources_ifdef(CONFIG_DESKTOP_FN_KEYS_ENABLE app PRIVATE fn_keys.c)

target_sources_ifdef(CONFIG_DESKTOP_WATCHDOG_ENABLE app PRIVATE watchdog.c)

target_sources_ifdef(CONFIG_DESKTOP_BAS_ENABLE app PRIVATE bas.c)

target_sources_ifdef(CONFIG_DESKTOP_QOS_ENABLE app PRIVATE qos.c)

target_sources_ifdef(CONFIG_DESKTOP_DEV_DESCR_ENABLE app PRIVATE dev_descr.c)

target_sources_ifdef(CONFIG_DESKTOP_HIDS_ENABLE app PRIVATE hids.c)

target_sources_ifdef(CONFIG_DESKTOP_HFCLK_LOCK_ENABLE app PRIVATE hfclk_lock.c)

target_sources_ifdef(CONFIG_DESKTOP_CONSTLAT_ENABLE app PRIVATE constlat.c)

target_sources_ifdef(CONFIG_DESKTOP_FAILSAFE_ENABLE app PRIVATE failsafe.c)

target_sources_ifdef(CONFIG_DESKTOP_CPU_MEAS_ENABLE app PRIVATE cpu_meas.c)

target_sources_ifdef(CONFIG_DESKTOP_NRF_PROFILER_SYNC_GPIO_ENABLE app PRIVATE nrf_profiler_sync.c)

target_sources_ifdef(CONFIG_DESKTOP_DVFS app PRIVATE dvfs.c)
