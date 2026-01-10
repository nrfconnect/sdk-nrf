#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_MCUBOOT_EXTRA_IMAGES AND NOT SB_CONFIG_PARTITION_MANAGER)
  include(image_flasher.cmake)

  foreach(i RANGE 1 ${SB_CONFIG_MCUBOOT_EXTRA_IMAGES})
    add_image_flasher(NAME dfu_extra_${i} HEX_FILE "${CMAKE_BINARY_DIR}/ext_img${i}.signed.hex")
  endforeach()
endif()
