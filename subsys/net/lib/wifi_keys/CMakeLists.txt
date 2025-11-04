#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(wifi_keys_driver)
zephyr_library_sources(wifi_keys.c)
target_link_libraries(wifi_keys_driver PRIVATE cracen_psa_driver)
zephyr_include_directories(.)
