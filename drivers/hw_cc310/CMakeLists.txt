#
# Copyright (c) 2019-2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
if(CONFIG_HW_CC310 OR CONFIG_HW_CC310_INTERRUPT)
  zephyr_library()
  zephyr_library_sources(hw_cc310.c)

  # Link with the nrf_cc310 platform library
  zephyr_library_link_libraries(platform_cc310)
endif()
