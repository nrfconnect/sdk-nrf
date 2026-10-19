#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DECT_NR_RPC_NET_IF
  dect_rpc_if.c
  dect_rpc_client_ping.c
  dect_rpc_client_shell_line.c
)
zephyr_library_sources_ifdef(CONFIG_DECT_NR_RPC_CONN_MGR dect_rpc_conn_mgr.c)
zephyr_library_sources_ifdef(CONFIG_DECT_NR_RPC_CLIENT_SHELL dect_rpc_shell.c)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)
