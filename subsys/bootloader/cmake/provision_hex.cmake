#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This CMake code adds commands for generating provision*.hex
# files. It can only be executed by the parent image.
#
# The provision*.hex files contain provisioned data for the bootloader.

if(CONFIG_SECURE_BOOT)
  include(${CMAKE_CURRENT_LIST_DIR}/provision_bl_storage_hex.cmake)
endif()

if(CONFIG_SECURE_BOOT OR CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
  include(${CMAKE_CURRENT_LIST_DIR}/provision_counters_hex.cmake)
endif()
