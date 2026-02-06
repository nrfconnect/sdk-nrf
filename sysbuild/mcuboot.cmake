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

    if(SB_CONFIG_PARTITION_MANAGER)
      ExternalZephyrVariantProject_Add(
        APPLICATION ${image}
        SOURCE_APP ${primary_image}
        SNIPPET slot1-partition
        BUILD_ONLY TRUE
      )
    else()
      ExternalZephyrVariantProject_Add(
        APPLICATION ${image}
        SOURCE_APP ${primary_image}
        SNIPPET secondary-app-partition
      )
    endif()

    UpdateableImage_Add(APPLICATION ${image} GROUP "VARIANT")

    if(SB_CONFIG_PARTITION_MANAGER)
      set_property(GLOBAL APPEND PROPERTY
          PM_APP_IMAGES
          "${image}"
      )
    endif()

  endforeach()
endif()
