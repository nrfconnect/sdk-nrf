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
      ExternalNcsVariantProject_Add(APPLICATION ${primary_image} VARIANT ${image})

      set_property(GLOBAL APPEND PROPERTY
          PM_APP_IMAGES
          "${image}"
      )
    else()
      # TODO: NCSDK-33774 add a general way to pass configuration options to the variant
      # image
      if(${primary_image} STREQUAL ${DEFAULT_IMAGE})
        zephyr_get(extra_conf_file SYSBUILD LOCAL VAR EXTRA_CONF_FILE)
      else()
        # All variables with ${image}_ prefix are passed to the variant image automatically.
        set(extra_conf_file)
      endif()

      ExternalNcsVariantProject_Add(APPLICATION ${primary_image} VARIANT ${image} SPLIT_KCONFIG true)
      UpdateableImage_Add(APPLICATION ${image} GROUP "VARIANT")
      set(${image}_EXTRA_CONF_FILE "${extra_conf_file}" CACHE INTERNAL "")
      set_target_properties(${image} PROPERTIES
        IMAGE_CONF_SCRIPT ${ZEPHYR_BASE}/share/sysbuild/image_configurations/MAIN_image_default.cmake
      )
      set(secondary_overlay "${ZEPHYR_NRF_MODULE_DIR}/subsys/mcuboot/mcuboot_secondary_app.overlay")

      add_overlay_dts(${image} "${secondary_overlay}")
    endif()
  endforeach()
endif()
