#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_FLASH_RPC_SYS_INIT flash_rpc_common.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_RPC_HOST flash_rpc_host.c)
zephyr_library_sources_ifdef(CONFIG_FLASH_RPC_CONTROLLER flash_rpc_controller.c)
