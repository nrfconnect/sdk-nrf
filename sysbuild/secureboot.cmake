#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)
if(SB_CONFIG_SECURE_BOOT)
  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    # Calculate the network board target
    string(REPLACE "/" ";" split_board_qualifiers ";${BOARD_QUALIFIERS}")
    list(GET split_board_qualifiers 1 target_soc)
    list(GET split_board_qualifiers 2 target_cpucluster)

    set(board_target_netcore "${BOARD}/${target_soc}/${SB_CONFIG_SECURE_BOOT_NETWORK_BOARD_TARGET_CPUCLUSTER}")

    set(target_soc)
    set(target_cpucluster)

    if(SB_CONFIG_PARTITION_MANAGER)
      ExternalZephyrProject_Add(
        APPLICATION b0n
        SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/netboot_deprecated_pm
        BOARD ${board_target_netcore}
        BOARD_REVISION ${BOARD_REVISION}
        BUILD_ONLY true
      )

      if(NOT "CPUNET" IN_LIST PM_DOMAINS)
        list(APPEND PM_DOMAINS CPUNET)
      endif()

      set_property(GLOBAL APPEND PROPERTY
        PM_CPUNET_IMAGES
        "b0n"
      )
    else()
      ExternalZephyrProject_Add(
        APPLICATION b0n
        SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/netboot
        BOARD ${board_target_netcore}
        BOARD_REVISION ${BOARD_REVISION}
      )

      add_overlay_dts(${SB_CONFIG_NETCORE_IMAGE_NAME}
        ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s0-partition.overlay
      )

      set(b0n_SIGNING_SCRIPT
        "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/b0n_provision_merge.cmake" CACHE INTERNAL
        "b0n provision merging script" FORCE
      )

      if(SB_CONFIG_NETCORE_APP_UPDATE)
        # PCD requires the offset of the s0 partition which is read from the b0n image, therefore
        # ensure that the b0n image is configured before the main application (including variant if
        # in direct-xip mode) and b0n
        sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} b0n)
        sysbuild_add_dependencies(CONFIGURE mcuboot b0n)

        if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
          sysbuild_add_dependencies(CONFIGURE mcuboot_secondary_app b0n)
        endif()
      endif()
    endif()

    set_target_properties(b0n PROPERTIES
      IMAGE_CONF_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/image_configurations/b0_image_default.cmake
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
      # Two-stage secure boot (NSIB + MCUboot) without Partition Manager needs a
      # per-board DTS partition layout, looked up by board target name so any SoC
      # can add its own under sysbuild/overlays/nsib-dts-2stage/ with no code change.
      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        string(REPLACE "/" "_" sb_board_norm "${BOARD}/${BOARD_QUALIFIERS}")
        set(sb_two_stage_overlay
          ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/nsib-dts-2stage/${sb_board_norm}.overlay)
        if(NOT EXISTS ${sb_two_stage_overlay})
          message(FATAL_ERROR
            "Two-stage secure boot (NSIB + MCUboot) without Partition Manager requires "
            "a DTS partition layout for this board, but none was found.\n"
            "Expected: ${sb_two_stage_overlay}")
        endif()
      endif()

      ExternalZephyrProject_Add(
        APPLICATION b0
        SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/bootloader
      )

      include(image_flasher.cmake)
      add_image_flasher(NAME app_provision HEX_FILE "${CMAKE_BINARY_DIR}/app_provision.hex" BASE_IMAGE b0)
      if(SB_CONFIG_SOC_SERIES_NRF54L)
        add_image_flasher(NAME bootconf HEX_FILE "${CMAKE_BINARY_DIR}/bootconf.hex" BASE_IMAGE b0)
        sysbuild_add_dependencies(FLASH bootconf b0)
      endif()

      # Single-stage NSIB (no MCUboot): alias s0/s1 onto the application slots so
      # bl_boot/bl_validation resolve the next-stage image from devicetree.
      # Two-stage: apply the full layout so b0 sees the MCUboot copies as s0/s1.
      if(NOT SB_CONFIG_BOOTLOADER_MCUBOOT)
        add_overlay_dts(b0 ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/nsib-dts-slots.overlay)
      else()
        add_overlay_dts(b0 ${sb_two_stage_overlay})
      endif()
    endif()

    set_target_properties(b0 PROPERTIES
      IMAGE_CONF_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/image_configurations/b0_image_default.cmake
    )

    sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} b0)
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
        add_overlay_dts(mcuboot_s1_variant ${sb_two_stage_overlay})

        add_overlay_dts(mcuboot ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s0-partition.overlay)
        add_overlay_dts(mcuboot ${sb_two_stage_overlay})

        # The application links into slot0_partition (image-0); apply the same
        # layout so its slots resolve to the two-stage addresses.
        add_overlay_dts(${DEFAULT_IMAGE} ${sb_two_stage_overlay})
      else()
        ExternalZephyrVariantProject_Add(
          APPLICATION ${DEFAULT_IMAGE}_s1_variant
          SOURCE_APP ${DEFAULT_IMAGE}
          EXTRA_DTC_OVERLAY_FILE ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s1-partition.overlay
        )

        add_overlay_dts(${DEFAULT_IMAGE}_s1_variant
          ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/nsib-dts-slots.overlay
        )

        add_overlay_dts(${DEFAULT_IMAGE}
          ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/s0-partition.overlay
        )
        add_overlay_dts(${DEFAULT_IMAGE}
          ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/overlays/nsib-dts-slots.overlay
        )
      endif()
    endif()
  endif()
endif()

if(SB_CONFIG_PARTITION_MANAGER)
  set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
endif()
