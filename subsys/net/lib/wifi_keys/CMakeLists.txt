#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(wifi_keys_driver)
zephyr_library_sources(wifi_keys.c)
target_include_directories(psa_crypto_library_config INTERFACE .)
target_link_libraries(psa_core PRIVATE wifi_keys_driver)
