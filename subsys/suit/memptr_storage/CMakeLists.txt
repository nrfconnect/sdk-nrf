#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(suit_memptr_storage_interface)
target_include_directories(suit_memptr_storage_interface INTERFACE include)
target_link_libraries(suit_memptr_storage_interface INTERFACE suit_platform_err)

zephyr_library()

zephyr_library_sources(src/suit_memptr_storage.c)

zephyr_library_link_libraries(suit_memptr_storage_interface)
