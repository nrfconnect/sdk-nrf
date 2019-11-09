#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_sources(hw_cc310.c)

# Link with the nrf_cc310 platform library
zephyr_library_link_libraries(platform_cc310)
