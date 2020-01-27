#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_BT_GATT_POOL gatt_pool.c)
zephyr_sources_ifdef(CONFIG_BT_GATT_DM gatt_dm.c)
zephyr_sources_ifdef(CONFIG_BT_SCAN scan.c)
zephyr_sources_ifdef(CONFIG_BT_CONN_CTX conn_ctx.c)
zephyr_sources_ifdef(CONFIG_BT_ENOCEAN enocean)

add_subdirectory_ifdef(CONFIG_BT_LL_NRFXLIB controller)
add_subdirectory_ifdef(CONFIG_BT_MESH_NRF_MODELS mesh)

add_subdirectory_ifdef(CONFIG_BT_NRF_SERVICES services)
