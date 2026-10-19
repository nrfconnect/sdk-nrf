#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DECT_NR_RPC_NET_IF
  dect_rpc_if.c
  dect_rpc_server_ping.c
)
zephyr_library_sources_ifdef(CONFIG_DECT_NR_RPC_SHELL dect_rpc_server_shell_line.c)

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
zephyr_library_sources_ifdef(CONFIG_DECT_NR_RPC_SERVER_SHELL_RPC_INIT dect_rpc_server_shell.c)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

zephyr_library_link_libraries(dect_utils_lib)

zephyr_library_link_libraries_ifdef(CONFIG_DECT_NR_RPC_SHELL dect_l2_shell_lib)
