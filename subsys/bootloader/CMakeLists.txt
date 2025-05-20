#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(bl_boot)

zephyr_include_directories(include)

if(NOT IMAGE_NAME AND NOT CONFIG_PARTITION_MANAGER_ENABLED)
  message(WARNING "Building bootloader without associated application. "
          "Dummy values will be used for partition configuration."
          "Building bootloader without associated application. ")

  zephyr_include_directories(include/dummy_values/)
endif()
