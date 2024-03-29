#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Module holding current SUIT execution mode
zephyr_interface_library_named(suit_execution_mode)
target_include_directories(suit_execution_mode INTERFACE include)
target_link_libraries(suit_execution_mode INTERFACE suit_platform_err)

zephyr_library()
zephyr_library_sources(src/suit_execution_mode.c)

zephyr_library_link_libraries(suit_execution_mode)
