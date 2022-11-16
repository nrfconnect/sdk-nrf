#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${CMAKE_CURRENT_LIST_DIR}/extensions.cmake)

if(SB_CONFIG_SECURE_BOOT)
  set(secure_boot_image_dir ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image)
  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    add_overlay_config(
      ${SB_CONFIG_NETCORE_HCI_RPMSG_NAME} # ToDo: Create a common Kconfig setting
                                          # which can then default to the exact
                                          # remote image selected, as to work for
                                          # hci, mpsl, thread, etc.
      "${secure_boot_image_dir}/secure_boot.conf"
    )

    add_overlay_config(mcuboot "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf")
  else()
    if (SB_CONFIG_BOOTLOADER_MCUBOOT)
      add_overlay_config(mcuboot ${secure_boot_image_dir}/multi_image_mcuboot.conf)

      # Include a kconfig file which enables CONFIG_FW_INFO in the image
      # which is booted by B0.
      add_overlay_config(mcuboot ${secure_boot_image_dir}/fw_info.conf)
    endif()
  endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/netcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/secureboot.cmake)
