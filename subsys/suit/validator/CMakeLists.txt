#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# SUIT Validator API
zephyr_interface_library_named(suit_validator)
target_include_directories(suit_validator INTERFACE include)
target_link_libraries(suit_validator INTERFACE suit_platform_err)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SUIT_VALIDATOR_IMPL_NRF54H20_SDFW src/suit_validator_nrf54h20.c)

zephyr_library_link_libraries(suit_validator)
zephyr_library_link_libraries(suit_memory_layout_interface)
