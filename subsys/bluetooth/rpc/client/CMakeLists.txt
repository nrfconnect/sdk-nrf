#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_include_directories(zephyr_interface BEFORE INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_sources(bt_rpc_gap_cli.c)
zephyr_sources_ifdef(CONFIG_BT_CONN bt_rpc_conn_cli.c)
zephyr_sources_ifdef(CONFIG_BT_CONN bt_rpc_gatt_svc_cli.c)
zephyr_sources_ifdef(CONFIG_BT_CONN bt_rpc_gatt_dis_cli.c)

