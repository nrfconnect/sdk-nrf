#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_BT_AMS_CLIENT ams_client.c)
zephyr_sources_ifdef(CONFIG_BT_ANCS_CLIENT ancs_client.c
                                           ancs_attr_parser.c
                                           ancs_app_attr_get.c)
zephyr_sources_ifdef(CONFIG_BT_BAS_CLIENT bas_client.c)
zephyr_sources_ifdef(CONFIG_BT_BMS bms.c)
zephyr_sources_ifdef(CONFIG_BT_CTS_CLIENT cts_client.c)
zephyr_sources_ifdef(CONFIG_BT_DFU_SMP dfu_smp.c)
zephyr_sources_ifdef(CONFIG_BT_GATTP gattp.c)
zephyr_sources_ifdef(CONFIG_BT_HIDS hids.c)
zephyr_sources_ifdef(CONFIG_BT_HOGP hogp.c)
zephyr_sources_ifdef(CONFIG_BT_THROUGHPUT throughput.c)
zephyr_sources_ifdef(CONFIG_BT_NSMS nsms.c)
zephyr_sources_ifdef(CONFIG_BT_NUS nus.c)
zephyr_sources_ifdef(CONFIG_BT_NUS_CLIENT nus_client.c)
zephyr_sources_ifdef(CONFIG_BT_LBS lbs.c)
zephyr_sources_ifdef(CONFIG_BT_LATENCY latency.c)
zephyr_sources_ifdef(CONFIG_BT_LATENCY_CLIENT latency_client.c)
zephyr_sources_ifdef(CONFIG_BT_RSCS rscs.c)
zephyr_sources_ifdef(CONFIG_BT_HRS_CLIENT hrs_client.c)
zephyr_sources_ifdef(CONFIG_BT_DDFS ddfs.c)
zephyr_sources_ifdef(CONFIG_BT_MDS mds.c)
add_subdirectory_ifdef(CONFIG_BT_CGMS cgms)
add_subdirectory_ifdef(CONFIG_BT_FAST_PAIR fast_pair)
add_subdirectory_ifdef(CONFIG_BT_WIFI_PROV wifi_prov)
