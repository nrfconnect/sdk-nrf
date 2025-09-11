#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(protocols_serialization_client)

# NORDIC SDK APP START
target_sources(app PRIVATE
    src/main.c
)

zephyr_library_sources_ifdef(CONFIG_OPENTHREAD_RPC src/ot_shell.c)
zephyr_library_sources_ifdef(CONFIG_MPSL_CX_SOFTWARE src/coex_shell.c)
zephyr_library_sources_ifdef(CONFIG_LOG_FORWARDER_RPC src/log_rpc_shell.c)
zephyr_library_sources_ifdef(CONFIG_NFC_RPC src/nfc_shell.c)
zephyr_library_sources_ifdef(CONFIG_BT_THROUGHPUT src/bt_throughput_shell.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS src/rpc_utils_shell.c)
zephyr_library_sources_ifdef(CONFIG_NRF_PS_CLIENT_BT_TEST src/bt_test_shell.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS_HEALTH_MONITOR src/health_monitor_shell.c)
# NORDIC SDK APP START
