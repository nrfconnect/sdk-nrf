#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS_CRASH_GEN crash_gen.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS_DEV_INFO dev_info.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS_DIE_TEMP die_temp.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS_REMOTE_SHELL remote_shell.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS_SYSTEM_HEALTH system_health.c)
zephyr_library_sources_ifdef(CONFIG_NRF_RPC_UTILS_HEALTH_MONITOR health_monitor.c)

zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../common
)
