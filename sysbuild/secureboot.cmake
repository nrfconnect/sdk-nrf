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

    ExternalZephyrProject_Add(
      APPLICATION b0n
      SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/netboot_deprecated_pm
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

    if(SB_CONFIG_SOC_SERIES_NRF53 AND SB_CONFIG_BOARD_IS_NON_SECURE)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_TFM_HAS_B0N y)
    else()
      set_config_bool(${DEFAULT_IMAGE} CONFIG_TFM_HAS_B0N n)
    endif()
  endif()

   if(SB_CONFIG_SECURE_BOOT_APPCORE)
    if(SB_CONFIG_PARTITION_MANAGER)
      ExternalZephyrProject_Add(
        APPLICATION b0
        SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/bootloader_deprecated_pm
        BUILD_ONLY true
      )

      if(NOT "APP" IN_LIST PM_DOMAINS)
        list(APPEND PM_DOMAINS APP)
      endif()
      set_property(GLOBAL APPEND PROPERTY
         PM_APP_IMAGES
         "b0"
      )
    else()
      ExternalZephyrProject_Add(
        APPLICATION b0
        SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/bootloader
      )

      include(image_flasher.cmake)
      add_image_flasher(NAME app_provision HEX_FILE "${CMAKE_BINARY_DIR}/app_provision.hex")
    endif()

    set_target_properties(b0 PROPERTIES
      IMAGE_CONF_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/image_configurations/b0_image_default.cmake
     )
   endif()

   if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
    if(SB_CONFIG_PARTITION_MANAGER)
      set(image s1_image)

      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        ExternalZephyrVariantProject_Add(
          APPLICATION ${image}
          SOURCE_APP mcuboot
          SNIPPET slot1-partition
          BUILD_ONLY TRUE
        )
      else()
        ExternalZephyrVariantProject_Add(
          APPLICATION ${image}
          SOURCE_APP ${DEFAULT_IMAGE}
          SNIPPET slot1-partition
          BUILD_ONLY TRUE
        )
      endif()

      set_property(GLOBAL APPEND PROPERTY
         PM_APP_IMAGES
         "${image}"
      )
    else()
      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        ExternalZephyrVariantProject_Add(
          APPLICATION mcuboot_s1_variant
          SOURCE_APP mcuboot
          EXTRA_DTC_OVERLAY_FILE ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s1-partition.overlay
        )

        add_overlay_dts(mcuboot ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s0-partition.overlay)
      else()
        ExternalZephyrVariantProject_Add(
          APPLICATION ${DEFAULT_IMAGE}_s1_variant
          SOURCE_APP ${DEFAULT_IMAGE}
          EXTRA_DTC_OVERLAY_FILE ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s1-partition.overlay
        )

        add_overlay_dts(${DEFAULT_IMAGE}
          ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s0-partition.overlay
        )
      endif()
    endif()
   endif()
 endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
if(SB_CONFIG_PARTITION_MANAGER)
  set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
endif()
