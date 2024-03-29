#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(suit_storage_interface)
target_include_directories(suit_storage_interface INTERFACE include)
target_link_libraries(suit_storage_interface INTERFACE suit_metadata)
target_link_libraries(suit_storage_interface INTERFACE suit_platform_err)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SUIT_STORAGE_LAYOUT_TEST src/suit_storage_test.c)
zephyr_library_sources_ifdef(CONFIG_SUIT_STORAGE_LAYOUT_NRF54H20 src/suit_storage_nrf54h20.c)
zephyr_library_sources(src/suit_storage_update.c)
zephyr_library_sources(src/suit_storage_encode.c)
zephyr_library_sources(src/suit_storage_envelope.c)
zephyr_library_sources(src/suit_storage_mpi.c)
zephyr_library_sources(src/suit_storage_nvv.c)
zephyr_library_sources(src/suit_storage_report.c)

zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_storage_interface)
