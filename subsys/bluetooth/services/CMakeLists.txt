#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_BT_BAS_CLIENT bas_client.c)
zephyr_sources_ifdef(CONFIG_BT_BMS bms.c)
zephyr_sources_ifdef(CONFIG_BT_DFU_SMP dfu_smp.c)
zephyr_sources_ifdef(CONFIG_BT_HIDS hids.c)
zephyr_sources_ifdef(CONFIG_BT_HOGP hogp.c)
zephyr_sources_ifdef(CONFIG_BT_THROUGHPUT throughput.c)
zephyr_sources_ifdef(CONFIG_BT_NUS nus.c)
zephyr_sources_ifdef(CONFIG_BT_NUS_CLIENT nus_client.c)
zephyr_sources_ifdef(CONFIG_BT_LBS lbs.c)
zephyr_sources_ifdef(CONFIG_BT_LATENCY latency.c)
zephyr_sources_ifdef(CONFIG_BT_LATENCY_CLIENT latency_client.c)
zephyr_sources_ifdef(CONFIG_BT_GADGETS gadgets.c)
