# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(suit_decrypt_test_utils)
target_include_directories(suit_decrypt_test_utils INTERFACE .)

zephyr_library_sources(decrypt_test_utils.c)
zephyr_library_link_libraries(suit_metadata)
