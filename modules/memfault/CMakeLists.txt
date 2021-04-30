#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(memfault_integration.c)
zephyr_library_sources_ifdef(
        CONFIG_MEMFAULT_NCS_USE_DEFAULT_METRICS
        memfault_ncs_metrics.c)
zephyr_library_sources_ifdef(
        CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP
        memfault_flash_coredump_storage.c)
zephyr_include_directories(config)
