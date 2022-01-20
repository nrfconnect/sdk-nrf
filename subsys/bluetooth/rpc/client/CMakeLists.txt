#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_link_libraries(subsys_bluetooth_rpc)
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources(
  bt_rpc_gap_client.c
  bt_rpc_crypto_client.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_CONN
  bt_rpc_conn_client.c
  bt_rpc_gatt_client.c
)

zephyr_library_sources(
  ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_RPC_INTERNAL_FUNCTIONS
  bt_rpc_internal_client.c
)
