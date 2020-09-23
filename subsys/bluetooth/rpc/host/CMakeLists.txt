#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources(bt_rpc_gap_host.c)
zephyr_sources_ifdef(CONFIG_BT_CONN bt_rpc_conn_host.c)
zephyr_sources_ifdef(CONFIG_BT_CONN bt_rpc_gatt_svc_host.c)
zephyr_sources_ifdef(CONFIG_BT_CONN bt_rpc_gatt_dis_host.c)

