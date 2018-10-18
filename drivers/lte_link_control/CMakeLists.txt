#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(CONFIG_LTE_LINK_CONTROL)
    zephyr_library()
    zephyr_library_sources(lte_lc.c)
    zephyr_library_include_directories(.)
endif()
