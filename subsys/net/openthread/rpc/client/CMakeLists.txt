#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  ot_rpc_if_client.c
  ot_rpc_instance_client.c
  ot_rpc_cli_client.c
  ot_rpc_coap_client.c
  ot_rpc_ctrl_client.c
  ot_rpc_commissioning_client.c
  ot_rpc_diag_client.c
  ot_rpc_message_client.c
  ot_rpc_udp_client.c
)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

zephyr_include_directories(${ZEPHYR_OPENTHREAD_MODULE_DIR}/include)
