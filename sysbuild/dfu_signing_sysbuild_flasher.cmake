#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_MCUBOOT_SYSBUILD_SIGN)
  include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/image_flasher.cmake)

  UpdateableImage_Get(images ALL)
  foreach(image ${images})
    add_image_flasher(NAME ${image}_flasher HEX_FILE "${CMAKE_BINARY_DIR}/${image}.signed.hex")
  endforeach()
endif()
