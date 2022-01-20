#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_link_libraries(subsys_bluetooth_rpc)

zephyr_library_sources(
  bt_rpc_gap_host.c
  bt_rpc_crypto_host.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_CONN
  bt_rpc_conn_host.c
  bt_rpc_gatt_host.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_RPC_INTERNAL_FUNCTIONS
  bt_rpc_internal_host.c
)

zephyr_library_include_directories_ifdef(
  CONFIG_BT_RPC_INTERNAL_FUNCTIONS
  ${ZEPHYR_BASE}/subsys/bluetooth/host
)
