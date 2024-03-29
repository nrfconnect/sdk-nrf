#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Module for usage in local domain to get information about the currently installed envelope

zephyr_interface_library_named(suit_envelope_info)
target_include_directories(suit_envelope_info INTERFACE include)

zephyr_library()
zephyr_library_sources(src/suit_envelope_info.c)

zephyr_library_link_libraries(suit_envelope_info)
zephyr_library_link_libraries(suit_platform_err)
zephyr_library_link_libraries(suit_utils)
