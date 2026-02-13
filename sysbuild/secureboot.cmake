#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)
if(SB_CONFIG_SECURE_BOOT)
  function(set_image_dtc_overlay_file image overlay_file)
    set(${image}_DTC_OVERLAY_FILE "${overlay_file}" CACHE STRING
      "DTC overlay file for ${image} image" FORCE
    )
  endfunction()

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

    if(SB_CONFIG_PARTITION_MANAGER)
      ExternalZephyrProject_Add(
        APPLICATION b0
        SOURCE_DIR ${secure_boot_source_dir}
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
        SOURCE_DIR ${secure_boot_source_dir}
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
        ExternalNcsVariantProject_Add(APPLICATION mcuboot VARIANT ${image})
      else()
        ExternalNcsVariantProject_Add(APPLICATION ${DEFAULT_IMAGE} VARIANT ${image})
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
          SNIPPET s1-partition
        )

        add_overlay_dts(mcuboot ${ZEPHYR_NRF_MODULE_DIR}/snippets/s0-partition/s0-partition.overlay)
      else()
        ExternalZephyrVariantProject_Add(
          APPLICATION ${DEFAULT_IMAGE}_s1_variant
          SOURCE_APP ${DEFAULT_IMAGE}
          SNIPPET s1-partition
        )

        add_overlay_dts(${DEFAULT_IMAGE}
          ${ZEPHYR_NRF_MODULE_DIR}/snippets/s0-partition/s0-partition.overlay
        )
      endif()
    endif()
   endif()
 endif()

set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
if(SB_CONFIG_PARTITION_MANAGER)
  set_property(GLOBAL PROPERTY PM_DOMAINS ${PM_DOMAINS})
endif()
