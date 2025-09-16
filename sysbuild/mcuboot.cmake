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

    ExternalNcsVariantProject_Add(APPLICATION ${primary_image} VARIANT ${image})

    if(SB_CONFIG_PARTITION_MANAGER)
      set_property(GLOBAL APPEND PROPERTY
          PM_APP_IMAGES
          "${image}"
      )
    else()
      UpdateableImage_Add(APPLICATION ${image} GROUP "VARIANT")

      string(CONFIGURE "${SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT_CUSTOM_OVERLAY}" secondary_overlay)
      if("${secondary_overlay}" STREQUAL "")
        set(secondary_overlay "${ZEPHYR_NRF_MODULE_DIR}/subsys/mcuboot/mcuboot_secondary_app.overlay")
      endif()

      add_overlay_dts(${image} "${secondary_overlay}")
    endif()

  endforeach()
endif()
