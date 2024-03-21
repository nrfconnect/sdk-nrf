#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  ot_rpc_cli_server.c
  ot_rpc_if_server.c
)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# TODO: add dependency on NET_L2_OPENTHREAD and remove the code below
zephyr_include_directories(${ZEPHYR_OPENTHREAD_MODULE_DIR}/include)
