#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_SPM)
  add_child_image(spm ${CMAKE_CURRENT_LIST_DIR}/nrf9160/spm)
endif()

if (CONFIG_SECURE_BOOT)
  add_child_image(b0 ${CMAKE_CURRENT_LIST_DIR}/bootloader)
endif ()
