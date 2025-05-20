#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT Events API
zephyr_interface_library_named(suit_events)
target_include_directories(suit_events INTERFACE include)

zephyr_library()
zephyr_library_sources(src/suit_events.c)
zephyr_library_link_libraries(suit_events)
