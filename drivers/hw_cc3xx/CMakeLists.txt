#
# Copyright (c) 2019-2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
if(CONFIG_HW_CC3XX)
  zephyr_library()
  zephyr_library_sources(hw_cc3xx.c)

  # Link with the nrf_cc3xx_platform library
  zephyr_library_link_libraries(platform_cc3xx)
endif()
