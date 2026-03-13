# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_FIRMWARE_LOADER_UPDATE)
  set(app_name ${SB_CONFIG_FIRMWARE_LOADER_INSTALLER_APPLICATION_IMAGE_NAME})

  ExternalZephyrProject_Add(
    APPLICATION ${app_name}
    SOURCE_DIR ${SB_CONFIG_FIRMWARE_LOADER_INSTALLER_APPLICATION_IMAGE_PATH}
    BUILD_ONLY y
  )

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    set_config_bool(${app_name} CONFIG_BOOTLOADER_MCUBOOT true)
  endif()

  if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
    set_config_bool(${app_name} CONFIG_NCS_MCUBOOT_BOOTLOADER_SIGN_MERGED_BINARY true)
  endif()
endif()
