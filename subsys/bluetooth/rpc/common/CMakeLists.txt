#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(include)

zephyr_sources(cbkproxy.c)
zephyr_sources(serialize.c)
zephyr_sources(bt_rpc_common.c)
#zephyr_sources_ifdef(CONFIG_BT_CONN bt_rpc_gatt_common.c)
