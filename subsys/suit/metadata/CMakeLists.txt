#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT Manifest Metadata types and conversion utilities
zephyr_interface_library_named(suit_metadata)
target_include_directories(suit_metadata INTERFACE include)
target_link_libraries(suit_metadata INTERFACE suit_platform_err)

zephyr_library()
zephyr_library_sources(src/suit_metadata.c)

zephyr_library_link_libraries(suit_metadata)
