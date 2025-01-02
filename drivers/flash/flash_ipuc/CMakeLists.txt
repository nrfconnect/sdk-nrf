#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(flash_ipuc.c)
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_ipuc)
