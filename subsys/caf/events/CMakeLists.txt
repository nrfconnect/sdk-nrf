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

zephyr_sources_ifdef(CONFIG_CAF_FACTORY_RESET_EVENTS
	factory_reset_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_FORCE_POWER_DOWN_EVENTS
	force_power_down_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_KEEP_ALIVE_EVENTS
	keep_alive_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_LED_EVENTS
	led_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_NET_STATE_EVENTS
	net_state_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_POWER_MANAGER_EVENTS
	power_manager_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_SENSOR_EVENTS
	sensor_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_BLE_SMP_TRANSFER_EVENTS
	ble_smp_event.c
)

zephyr_sources_ifdef(CONFIG_CAF_SENSOR_DATA_AGGREGATOR_EVENTS
	sensor_data_aggregator_event.c
)

zephyr_linker_sources(SECTIONS module_id_list.ld)
