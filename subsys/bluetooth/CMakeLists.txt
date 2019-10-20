#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_BT_GATT_POOL gatt_pool.c)
zephyr_sources_ifdef(CONFIG_BT_GATT_DM gatt_dm.c)
zephyr_sources_ifdef(CONFIG_BT_SCAN scan.c)
zephyr_sources_ifdef(CONFIG_BT_CONN_CTX conn_ctx.c)

add_subdirectory_ifdef(CONFIG_BT_LL_NRFXLIB controller)

if (CONFIG_BT_GATT_BAS_C     OR
    CONFIG_BT_GATT_DFU_SMP_C OR
    CONFIG_BT_GATT_HIDS      OR
    CONFIG_BT_GATT_HIDS_C    OR
    CONFIG_BT_GATT_LBS       OR
    CONFIG_BT_GATT_NUS       OR
    CONFIG_BT_GATT_NUS_C     OR
    CONFIG_BT_GATT_LATENCY   OR
    CONFIG_BT_GATT_LATENCY_C OR
    CONFIG_BT_GATT_THROUGHPUT)
	add_subdirectory(services)
endif()
