#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)

zephyr_library()

zephyr_library_sources(nrf_rpc_os.c)

zephyr_library_sources_ifdef(CONFIG_NRF_RPC_IPC_SERVICE nrf_rpc_ipc.c)

zephyr_library_sources_ifdef(CONFIG_NRF_RPC_SERIALIZE_API nrf_rpc_serialize.c)

zephyr_library_sources_ifdef(CONFIG_NRF_RPC_CALLBACK_PROXY nrf_rpc_cbkproxy.c)

zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UART_TRANSPORT nrf_rpc_uart.c)

zephyr_library_sources_ifdef(CONFIG_NRF_RPC_INIT nrf_rpc_init.c)

add_subdirectory_ifdef(CONFIG_NRF_RPC_UTILS rpc_utils)
