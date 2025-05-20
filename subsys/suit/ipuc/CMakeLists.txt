#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Module giving an access to in-place updateable components

zephyr_interface_library_named(suit_ipuc)
target_include_directories(suit_ipuc INTERFACE include)
target_link_libraries(suit_ipuc INTERFACE suit_platform_err)
target_link_libraries(suit_ipuc INTERFACE suit_metadata)

if(CONFIG_SUIT_IPUC_IMPL_SDFW)
  zephyr_library()
  zephyr_library_sources(src/suit_ipuc_sdfw.c)

  zephyr_library_link_libraries(suit_ipuc)
  zephyr_library_link_libraries(suit_utils)
  zephyr_library_link_libraries(suit_memory_layout_interface)
  zephyr_library_link_libraries(suit_source_selector_interface)
  zephyr_library_link_libraries(suit_stream_sources_interface)
endif()
