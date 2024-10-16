#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  ot_rpc_cli.c
  ot_rpc_coap.c
  ot_rpc_diag.c
  ot_rpc_dataset.c
  ot_rpc_instance.c
  ot_rpc_ip6.c
  ot_rpc_link.c
  ot_rpc_message.c
  ot_rpc_netdata.c
  ot_rpc_srp_client.c
  ot_rpc_thread.c
  ot_rpc_udp.c
)

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_RPC_NET_IF ot_rpc_if.c)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

zephyr_include_directories(${ZEPHYR_OPENTHREAD_MODULE_DIR}/include)
