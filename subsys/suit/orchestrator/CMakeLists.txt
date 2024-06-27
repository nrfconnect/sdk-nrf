#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT Orchestrator API
zephyr_interface_library_named(suit_orchestrator_interface)
target_include_directories(suit_orchestrator_interface INTERFACE include)

# SUIT Orchestrator implementation for SDFW
zephyr_library()
zephyr_library_sources(src/suit_orchestrator_sdfw.c)

zephyr_library_link_libraries(suit)
zephyr_library_link_libraries(suit_orchestrator_interface)
zephyr_library_link_libraries(suit_storage_interface)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_mci)
zephyr_library_link_libraries(suit_platform_err)
zephyr_library_link_libraries(suit_execution_mode)
zephyr_library_link_libraries(suit_cache_interface)
