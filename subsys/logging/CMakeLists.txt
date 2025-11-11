#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_LOG_FORWARDER_RPC OR CONFIG_LOG_BACKEND_RPC)
  zephyr_library()
  zephyr_library_sources_ifdef(CONFIG_LOG_FORWARDER_RPC log_forwarder_rpc.c)
  zephyr_library_sources_ifdef(CONFIG_LOG_BACKEND_RPC log_backend_rpc.c)
  zephyr_library_sources_ifdef(CONFIG_LOG_BACKEND_RPC_HISTORY_STORAGE_RAM log_backend_rpc_history_ram.c)
  zephyr_library_sources_ifdef(CONFIG_LOG_BACKEND_RPC_HISTORY_STORAGE_FCB log_backend_rpc_history_fcb.c)
endif()
