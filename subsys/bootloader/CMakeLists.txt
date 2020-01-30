#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory(provision)

zephyr_include_directories(include)

if (NOT IMAGE_NAME)
  message(WARNING "Building bootloader without associated application. "
          "Dummy values will be used for partition configuration."
          "The idiomatic way of including the immutable bootloader is to set "
          "the configuration 'CONFIG_SECURE_BOOTLOADER=y' in your applications "
          "'prj.conf'.")
  zephyr_include_directories(include/dummy_values/)
endif()

if (DEFINED CONFIG_SOC_NRF9160)
  message(WARNING "
      --------------------------------------------------------
      --- WARNING: When using the immutable bootloader on  ---
      --- nRF9160, the UICR must be erased when flashing.  ---
      --- E.g. by calling 'west flash --erase'             ---
      --------------------------------------------------------")
endif()
