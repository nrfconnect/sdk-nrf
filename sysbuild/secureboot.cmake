#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)
if(SB_CONFIG_SECURE_BOOT)
  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    # Calculate the network board target
    string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
    list(GET split_board_qualifiers 1 target_soc)
    list(GET split_board_qualifiers 2 target_cpucluster)

    set(board_target_netcore "${BOARD}/${target_soc}/${SB_CONFIG_SECURE_BOOT_NETWORK_BOARD_TARGET_CPUCLUSTER}")

    set(target_soc)
    set(target_cpucluster)

    set(secure_boot_source_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/netboot)

    ExternalZephyrProject_Add(
      APPLICATION b0n
      SOURCE_DIR ${secure_boot_source_dir}
      BOARD ${board_target_netcore}
      BOARD_REVISION ${BOARD_REVISION}
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
  endif()

  if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
    set(image s1_image)

    if(SB_CONFIG_PARTITION_MANAGER)
      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        ExternalNcsVariantProject_Add(APPLICATION mcuboot VARIANT ${image})
      else()
        ExternalNcsVariantProject_Add(APPLICATION ${DEFAULT_IMAGE} VARIANT ${image})
      endif()
    else()
      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        ExternalNcsVariantProject_Add(APPLICATION mcuboot VARIANT ${image} SPLIT_KCONFIG true)
        # TODO: NCSDK-33774 - add a robust way of passing all the required Kconfig options from MCUBOOT to s1 image
        set_target_properties(${image} PROPERTIES
          IMAGE_CONF_SCRIPT ${ZEPHYR_BASE}/share/sysbuild/image_configurations/BOOTLOADER_image_default.cmake
        )
        set_config_int(${image} CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER ${SB_CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER})
        set_config_int(${image} CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER ${SB_CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER})
        math(EXPR mcuboot_total_images "${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES} + ${SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES}")
        set_config_int(${image} CONFIG_UPDATEABLE_IMAGE_NUMBER ${mcuboot_total_images})
      else()
        ExternalNcsVariantProject_Add(APPLICATION ${DEFAULT_IMAGE} VARIANT ${image} SPLIT_KCONFIG true)
      endif()
      set(slot_overlay_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/bootloader/sysbuild)
      cmake_path(APPEND slot_overlay_dir "s1_image.overlay" OUTPUT_VARIABLE s1_overlay)
      add_overlay_dts(${image} "${s1_overlay}")
      set_config_bool(${image} CONFIG_SECURE_BOOT y)
      set_config_bool(${image} CONFIG_FW_INFO y)
    endif()

    set_property(GLOBAL APPEND PROPERTY
        PM_APP_IMAGES
        "${image}"
    )
  endif()
endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
