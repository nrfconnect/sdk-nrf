#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(memfault_integration.c)

zephyr_library_include_directories(include)

zephyr_library_sources_ifdef(
        CONFIG_MEMFAULT_NCS_USE_DEFAULT_METRICS
        memfault_ncs_metrics.c)

zephyr_library_sources_ifdef(
        CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP
        memfault_flash_coredump_storage.c)

zephyr_library_sources_ifdef(
        CONFIG_MEMFAULT_NCS_LTE_METRICS
        memfault_lte_metrics.c
)

zephyr_library_sources_ifdef(
        CONFIG_MEMFAULT_NCS_BT_METRICS
        memfault_bt_metrics.c)

zephyr_library_sources_ifdef(
        CONFIG_MEMFAULT_NCS_ETB_CAPTURE
        memfault_etb_trace_capture.c)

zephyr_include_directories_ifdef(
        CONFIG_MEMFAULT_METRICS_EXTRA_DEFS_FILE
        config)

zephyr_include_directories_ifdef(
        CONFIG_MEMFAULT_PLATFORM_EXTRA_CONFIG_FILE
        config)
