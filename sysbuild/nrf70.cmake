#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE AND NOT SB_CONFIG_PARTITION_MANAGER)
  include(image_flasher.cmake)

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    add_image_flasher(NAME nrf70 HEX_FILE "${CMAKE_BINARY_DIR}/nrf70.signed.hex")
  else()
    add_image_flasher(NAME nrf70 HEX_FILE "${CMAKE_BINARY_DIR}/nrf70.hex")
  endif()
endif()
