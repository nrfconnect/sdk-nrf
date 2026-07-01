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

    set_target_properties(b0 PROPERTIES
      IMAGE_CONF_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/image_configurations/b0_image_default.cmake
    )

    sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} b0)
  endif()

  if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
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
