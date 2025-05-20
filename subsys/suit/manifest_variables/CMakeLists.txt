#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Module giving an access to manifest-accessible variables and configuration data
zephyr_interface_library_named(suit_manifest_variables)
target_include_directories(suit_manifest_variables INTERFACE include)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SUIT_MANIFEST_VARIABLES_IMPL_SDFW src/suit_manifest_variables.c)

zephyr_library_link_libraries(suit_manifest_variables)
zephyr_library_link_libraries(suit_platform_err)
zephyr_library_link_libraries(suit_storage_interface)
