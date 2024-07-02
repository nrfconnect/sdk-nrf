#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set_config_bool(${image} CONFIG_BOOTLOADER_MCUBOOT y)
  set_config_bool(${image} CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER y)
  set_config_string(${image} CONFIG_MCUBOOT_SIGNATURE_KEY_FILE "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
  set_config_string(${image} CONFIG_MCUBOOT_ENCRYPTION_KEY_FILE "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")

  if("${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
    set_config_bool(${image} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE y)
  else()
    set_config_bool(${image} CONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE n)
  endif()

  if(SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES)
    set_property(TARGET ${image} APPEND_STRING PROPERTY CONFIG "CONFIG_UPDATEABLE_IMAGE_NUMBER=${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES}\n")
  endif()

  if(SB_CONFIG_PARTITION_MANAGER)
    set_target_properties(${image} PROPERTIES BUILD_ONLY true)
  endif()

  if(SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK)
    set_config_bool(${image} CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK y)
  endif()
endif()
