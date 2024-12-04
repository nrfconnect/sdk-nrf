#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(src/suit_recovery_button.c)

zephyr_library_link_libraries(suit_platform_err)
zephyr_library_link_libraries(suit_utils)
