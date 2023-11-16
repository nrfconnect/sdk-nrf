#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)
if(SB_CONFIG_SECURE_BOOT)
  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    set(secure_boot_source_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/netboot)

    ExternalZephyrProject_Add(
      APPLICATION b0n
      SOURCE_DIR ${secure_boot_source_dir}
      BOARD ${SB_CONFIG_SECURE_BOOT_NETWORK_BOARD}
      BUILD_ONLY true
    )
    set_target_properties(b0n PROPERTIES
      IMAGE_CONF_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/image_configurations/b0_image_default.cmake
    )

    if(NOT "CPUNET" IN_LIST PM_DOMAINS)
      list(APPEND PM_DOMAINS CPUNET)
    endif()
    set_property(GLOBAL APPEND PROPERTY
        PM_CPUNET_IMAGES
        "b0n"
    )

    if(SB_CONFIG_SECURE_BOOT_DOMAIN_APP)
      set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET
                   "b0n"
      )
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT_APPCORE)
    set(secure_boot_source_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/bootloader)

    ExternalZephyrProject_Add(
      APPLICATION b0
      SOURCE_DIR ${secure_boot_source_dir}
      BUILD_ONLY true
    )
    set_target_properties(b0 PROPERTIES
      IMAGE_CONF_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/image_configurations/b0_image_default.cmake
    )

    if(NOT "APP" IN_LIST PM_DOMAINS)
      list(APPEND PM_DOMAINS APP)
    endif()
    set_property(GLOBAL APPEND PROPERTY
        PM_APP_IMAGES
        "b0"
    )

    if(SB_CONFIG_SECURE_BOOT_DOMAIN_APP)
      set_property(GLOBAL PROPERTY DOMAIN_APP_APP
                   "b0"
      )
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
    set(image s1_image)

    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      ExternalNcsVariantProject_Add(APPLICATION mcuboot VARIANT ${image})
    else()
      ExternalNcsVariantProject_Add(APPLICATION ${DEFAULT_IMAGE} VARIANT ${image})
    endif()

    set_property(GLOBAL APPEND PROPERTY
        PM_APP_IMAGES
        "${image}"
    )
  endif()
endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
