#
# Copyright (c) 2018 Nordic Semiconductor
# Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
zephyr_sources(event_manager.c)
zephyr_sources_ifdef(CONFIG_SHELL event_manager_shell.c)
zephyr_sources_ifdef(CONFIG_EVENT_MANAGER_STORAGE event_manager_storage.c)

if (CONFIG_EVENT_MANAGER_STORAGE)
    ncs_add_partition_manager_config(pm.events.yml)
    zephyr_sources_ifdef(CONFIG_EVENT_MANAGER_STORAGE_BACKEND_NVS event_manager_storage_nvs.c)
    zephyr_sources_ifdef(CONFIG_EVENT_MANAGER_STORAGE_BACKEND_LITTLEFS event_manager_storage_littlefs.c)
endif ()

zephyr_linker_sources(SECTIONS em.ld)
