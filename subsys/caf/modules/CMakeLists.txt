#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_CAF_BLE_ADV ble_adv.c)

zephyr_sources_ifdef(CONFIG_CAF_BLE_STATE ble_state.c)

zephyr_sources_ifdef(CONFIG_CAF_BUTTONS buttons.c)

zephyr_sources_ifdef(CONFIG_CAF_CLICK_DETECTOR click_detector.c)

zephyr_sources_ifdef(CONFIG_CAF_LEDS leds.c)

zephyr_sources_ifdef(CONFIG_CAF_SENSOR_SAMPLER sensor_sampler.c)

zephyr_sources_ifdef(CONFIG_CAF_BLE_SMP ble_smp.c)
zephyr_link_libraries_ifdef(CONFIG_CAF_BLE_SMP MCUMGR)
