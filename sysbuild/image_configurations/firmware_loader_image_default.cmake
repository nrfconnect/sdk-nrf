#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  if(SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES)
    set_config_int(${image} CONFIG_UPDATEABLE_IMAGE_NUMBER ${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES})
  endif()

  if(SB_CONFIG_PARTITION_MANAGER)
    set_target_properties(${image} PROPERTIES BUILD_ONLY true)
  endif()

  if(SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK)
    set_config_bool(${image} CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK y)
  endif()
endif()
