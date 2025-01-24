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

if(CONFIG_SUIT_VALIDATOR_IMPL_SDFW)
  zephyr_library_sources(src/suit_validator_${CONFIG_SOC}.c)
endif()

zephyr_library_link_libraries(suit_validator)
zephyr_library_link_libraries(suit_memory_layout_interface)
