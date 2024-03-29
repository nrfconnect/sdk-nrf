#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(suit_memory_layout_interface)
target_include_directories(suit_memory_layout_interface INTERFACE include)

zephyr_library()
zephyr_library_sources(src/suit_memory_layout.c)

zephyr_library_link_libraries(suit_memory_layout_interface)
