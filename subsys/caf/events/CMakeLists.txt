#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_CAF_MODULE_STATE_EVENTS
	module_state_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_PM_EVENTS
	power_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_BLE_COMMON_EVENTS
	ble_common_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_BUTTON_EVENTS
	button_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_CLICK_EVENTS
	click_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_LED_EVENTS
	led_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_SENSOR_EVENTS
	sensor_event.c
)
