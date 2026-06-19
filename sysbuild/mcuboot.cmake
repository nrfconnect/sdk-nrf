#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
  UpdateableImage_Get(images GROUP "DEFAULT")

  foreach(primary_image ${images})
    if(${primary_image} STREQUAL ${DEFAULT_IMAGE})
      # Keep the old name for backward compatibility.
      set(image "mcuboot_secondary_app")
    else()
      set(image "${primary_image}_secondary_app")
    endif()

    ExternalZephyrVariantProject_Add(
      APPLICATION ${image}
      SOURCE_APP ${primary_image}
      SNIPPET secondary-app-partition
    )

    UpdateableImage_Add(APPLICATION ${image} GROUP "VARIANT")
  endforeach()
endif()

if(SB_CONFIG_BOOTLOADER_MCUBOOT AND SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
  # Downgrade prevention in MCUboot requires provisioning UICR with prepared counter structures.
  include(image_flasher.cmake)
  add_image_flasher(NAME app_provision HEX_FILE "${CMAKE_BINARY_DIR}/app_provision.hex" BASE_IMAGE mcuboot)
  sysbuild_add_dependencies(FLASH mcuboot app_provision)
endif()
