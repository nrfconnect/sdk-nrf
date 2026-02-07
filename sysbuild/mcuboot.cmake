#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

set(secondary_overlay "${ZEPHYR_NRF_MODULE_DIR}/subsys/mcuboot/mcuboot_secondary_app.overlay")

if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT AND SB_CONFIG_SOC_SERIES_NRF54H)
  UpdateableImage_Get(images GROUP "DEFAULT")

  foreach(primary_image ${images})
    if(${primary_image} STREQUAL ${DEFAULT_IMAGE})
      # Keep the old name for backward compatibility.
      set(image "mcuboot_secondary_app")
    else()
      set(image "${primary_image}_secondary_app")
    endif()

    ExternalNcsVariantProject_Add(APPLICATION ${primary_image} VARIANT ${image})

    UpdateableImage_Add(APPLICATION ${image} GROUP "VARIANT")
    add_overlay_dts(${image} "${secondary_overlay}")
  endforeach()
elseif(SB_CONFIG_MCUBOOT_DIRECT_XIP_GENERATE_VARIANT)
  UpdateableImage_Get(images GROUP "DEFAULT")

  foreach(primary_image ${images})
    if(SB_CONFIG_PARTITION_MANAGER)
      if(${primary_image} STREQUAL ${DEFAULT_IMAGE})
        # Keep the old name for backward compatibility.
        set(image "mcuboot_secondary_app")
      else()
        set(image "${primary_image}_secondary_app")
        add_overlay_dts(${image} "${secondary_overlay}")
      endif()
    else()
      set(image "${primary_image}_slot1_variant")
    endif()

    if(SB_CONFIG_PARTITION_MANAGER)
      ExternalNcsVariantProject_Add(APPLICATION ${primary_image} VARIANT ${image})

      set_property(GLOBAL APPEND PROPERTY
          PM_APP_IMAGES
          "${image}"
      )
    else()
      UpdateableImage_Add(APPLICATION ${image} GROUP "VARIANT")
    endif()
  endforeach()
endif()
