#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
  set(image mcuboot_secondary_app)
  if(SB_CONFIG_PARTITION_MANAGER)
    ExternalNcsVariantProject_Add(APPLICATION ${DEFAULT_IMAGE} VARIANT ${image})

    set_property(GLOBAL APPEND PROPERTY
        PM_APP_IMAGES
        "${image}"
    )
  else()
    # TODO: NCSDK-33774 add a general way to pass configuration options to the variant
    # image
    zephyr_get(extra_conf_file SYSBUILD LOCAL VAR EXTRA_CONF_FILE)

    ExternalNcsVariantProject_Add(APPLICATION ${DEFAULT_IMAGE} VARIANT ${image} SPLIT_KCONFIG true)
    set(${image}_EXTRA_CONF_FILE "${extra_conf_file}" CACHE INTERNAL "")
    set_target_properties(${image} PROPERTIES
      IMAGE_CONF_SCRIPT ${ZEPHYR_BASE}/share/sysbuild/image_configurations/MAIN_image_default.cmake
    )
    set(secondary_overlay "${ZEPHYR_NRF_MODULE_DIR}/subsys/mcuboot/mcuboot_secondary_app.overlay")
    add_overlay_dts(${image} "${secondary_overlay}")
  endif()
endif()
