#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(src/suit_orchestrator_app.c)

zephyr_library_link_libraries_ifdef(CONFIG_SUIT_PROCESSOR suit)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_STREAM_IPC_PROVIDER suit_stream_sources_interface)
zephyr_library_link_libraries_ifdef(CONFIG_SUIT_CACHE_RW suit_cache_interface)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_envelope_info)
zephyr_library_link_libraries(suit_platform_err)
