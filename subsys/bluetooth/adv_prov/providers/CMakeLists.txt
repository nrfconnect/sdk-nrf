#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_BT_ADV_PROV_FAST_PAIR	fast_pair.c)
zephyr_sources_ifdef(CONFIG_BT_ADV_PROV_FLAGS		flags.c)
zephyr_sources_ifdef(CONFIG_BT_ADV_PROV_GAP_APPEARANCE	gap_appearance.c)
zephyr_sources_ifdef(CONFIG_BT_ADV_PROV_DEVICE_NAME	device_name.c)
zephyr_sources_ifdef(CONFIG_BT_ADV_PROV_SWIFT_PAIR	swift_pair.c)
zephyr_sources_ifdef(CONFIG_BT_ADV_PROV_TX_POWER	tx_power.c)
