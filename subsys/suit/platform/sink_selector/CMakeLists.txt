#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_interface_library_named(suit_sink_selector_interface)
target_include_directories(suit_sink_selector_interface INTERFACE include)

zephyr_library()
zephyr_library_sources(src/suit_sdfw_sink_selector.c)

zephyr_library_link_libraries(suit)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_platform_interface)
zephyr_library_link_libraries(suit_stream_sinks_interface)
zephyr_library_link_libraries(suit_sink_selector_interface)
