#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_SPM)
  if(CONFIG_SPM_BUILD_STRATEGY_FROM_SOURCE)
    zephyr_add_external_image("spm" ${CMAKE_CURRENT_LIST_DIR}/nrf9160/spm)
  endif()
endif()

if (CONFIG_SECURE_BOOT)
  zephyr_add_external_image("b0" ${CMAKE_CURRENT_LIST_DIR}/bootloader)
endif ()
