#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  ot_rpc_cli_server.c
  ot_rpc_coap_server.c
  ot_rpc_ctrl_server.c
  ot_rpc_instance_server.c
  ot_rpc_message_server.c
)

zephyr_library_sources_ifdef(CONFIG_NET_L2_OPENTHREAD
  ot_rpc_if_server.c
  ot_rpc_commissioning_server.c
  ot_rpc_diag_server.c
  ot_rpc_udp_server.c
)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)
