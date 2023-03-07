#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This CMakeLists.txt is executed by the root application only when
# B0 (SECURE_BOOT) is enabled. First, figure out what image will be
# booted by B0, and set the required properties for that image to be
# bootable by B0.

if (CONFIG_NCS_IS_VARIANT_IMAGE)
  # When building the a variant of an image, the configuration of the variant image
  # is identical to the base image, except 'CONFIG_NCS_IS_VARIANT_IMAGE'. This file is
  # included if 'CONFIG_SECURE_BOOT' is set, which can be the case for the base
  # image. However, the logic in this file should only be performed once, for
  # the base image, not for the variant variant. Hence, we have this check to avoid
  # execution of this file for the variant image.
    return()
endif()

if (CONFIG_BOOTLOADER_MCUBOOT)
  set(image_to_boot mcuboot)

  add_overlay_config(
    mcuboot
    ${CMAKE_CURRENT_SOURCE_DIR}/multi_image_mcuboot.conf
    )
endif()

if (image_to_boot)
  # Include a kconfig file which enables CONFIG_FW_INFO in the image
  # which is booted by B0.
  add_overlay_config(
    ${image_to_boot}
    ${CMAKE_CURRENT_SOURCE_DIR}/fw_info.conf
    )
else()
  assert(CONFIG_FW_INFO "CONFIG_FW_INFO must be set")
endif()
