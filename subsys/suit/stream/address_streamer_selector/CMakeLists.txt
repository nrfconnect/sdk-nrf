#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(suit_source_selector_interface)
target_include_directories(suit_source_selector_interface INTERFACE include)

zephyr_library()
zephyr_library_sources(src/suit_address_streamer_selector.c)

zephyr_library_link_libraries(suit_stream_sources_interface)
zephyr_library_link_libraries(suit_source_selector_interface)
zephyr_library_link_libraries(suit_stream_sinks_interface)
