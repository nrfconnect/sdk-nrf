#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Usage:
#   configure_cache(IMAGE <name>)
#
# This function is similar in nature to configure file, except that it takes
# an image as argument to work on the CMakeCache of said image.
#
# The CMake cache of the image will only be re-written in case there are
# variables which has been updated as part of the call.
#
# Only variable of the form `@VAR_NAME@` will be updated.
#
# IMAGE: image name identifying the cache to update.
#
function(configure_cache)
  set(single_args IMAGE)
  cmake_parse_arguments(CONFIGURE_CACHE "" "${single_args}" "" ${ARGN})

  zephyr_check_arguments_required(configure_cache CONFIGURE_CACHE IMAGE)

  ExternalProject_Get_Property(${CONFIGURE_CACHE_IMAGE} BINARY_DIR)

  # Copy the cache file to avoid a dependency to the image's cache file.
  # A dependency on the image's cache file cause unneccessary CMake re-runs for
  # all sysbuild images whenever a setting in the cache changes, even for not
  # sysbuild relevant changes.
  zephyr_file_copy("${BINARY_DIR}/CMakeCache.txt" "${BINARY_DIR}/CMakeCache.tmp" ONLY_IF_DIFFERENT)
  configure_file("${BINARY_DIR}/CMakeCache.tmp" "${BINARY_DIR}/CMakeCache.txt.tmp" @ONLY)
  zephyr_file_copy("${BINARY_DIR}/CMakeCache.txt.tmp" "${BINARY_DIR}/CMakeCache.txt" ONLY_IF_DIFFERENT)
endfunction()

function(store_ncs_vars)
  get_property(ncs_vars DIRECTORY ${SYSBUILD_CURRENT_CMAKE_DIR} PROPERTY VARIABLES)
  list(FILTER ncs_vars INCLUDE REGEX "NCS_.*")
  foreach(var ${ncs_vars})
    set_property(GLOBAL APPEND PROPERTY NCS_VARS ${var})
    set_property(GLOBAL PROPERTY ${var} ${${var}})
  endforeach()
endfunction()

function(restore_ncs_vars)
  get_property(ncs_vars GLOBAL PROPERTY NCS_VARS)
  foreach(var ${ncs_vars})
    get_property(var_value GLOBAL PROPERTY ${var})
    set(${var} ${var_value} PARENT_SCOPE)
  endforeach()
endfunction()

function(include_provision_hex)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/provision_hex.cmake)
endfunction()

function(include_suit)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/suit.cmake)
endfunction()

function(include_packaging)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/b0_mcuboot_signing.cmake)
  include(${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/cmake/packaging.cmake)
endfunction()

function(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)
  cmake_parse_arguments(PRE_CMAKE "" "" "IMAGES" ${ARGN})
  restore_ncs_vars()

  if(NOT SB_CONFIG_PARTITION_MANAGER)
    # Disable any Kconfigs for child images in the main application
    foreach(config NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE NCS_INCLUDE_RPMSG_CHILD_IMAGE NCS_SAMPLE_REMOTE_SHELL_CHILD_IMAGE
            NCS_SAMPLE_DTM_REMOTE_HCI_CHILD_IMAGE NCS_SAMPLE_PERIPHERAL_RADIO_TEST_CHILD_IMAGE B0_BUILD_STRATEGY_USE_HEX_FILE
            B0_BUILD_STRATEGY_SKIP_BUILD B0_BUILD_STRATEGY_FROM_SOURCE PM_SINGLE_IMAGE MCUBOOT_BUILD_STRATEGY_USE_HEX_FILE
            MCUBOOT_BUILD_STRATEGY_SKIP_BUILD MCUBOOT_BUILD_STRATEGY_FROM_SOURCE)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_${config} n)
    endforeach()
  endif()

  foreach(image ${PRE_CMAKE_IMAGES})
    if(SB_CONFIG_PARTITION_MANAGER)
      set_config_bool(${image} CONFIG_PARTITION_MANAGER_ENABLED y)
    else()
      set_config_bool(${image} CONFIG_PARTITION_MANAGER_ENABLED n)
    endif()

    if(SB_CONFIG_BUILD_OUTPUT_BIN)
      set_config_bool(${image} CONFIG_BUILD_OUTPUT_BIN y)
    else()
      set_config_bool(${image} CONFIG_BUILD_OUTPUT_BIN n)
    endif()

    if(SB_CONFIG_BUILD_OUTPUT_HEX)
      set_config_bool(${image} CONFIG_BUILD_OUTPUT_HEX y)
    else()
      set_config_bool(${image} CONFIG_BUILD_OUTPUT_HEX n)
    endif()
  endforeach()

  if(SB_CONFIG_PARTITION_MANAGER AND SB_CONFIG_BOOTLOADER_MCUBOOT)
    if(SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY y)
      set_config_bool(mcuboot CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY y)
    else()
      set_config_bool(${DEFAULT_IMAGE} CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY n)
      set_config_bool(mcuboot CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY n)
    endif()
  endif()

  # Calculate the secure board target when building for non-secure board targets
  string(REPLACE "/" ";" split_board_qualifiers "${BOARD_QUALIFIERS}")
  list(GET split_board_qualifiers 1 target_soc)

  if(SB_CONFIG_SOC_SERIES_NRF53X OR SB_CONFIG_SOC_SERIES_NRF54LX)
    list(LENGTH split_board_qualifiers target_length)

    if("${target_length}" GREATER "3")
      list(GET split_board_qualifiers 2 target_cpucluster)
      list(GET split_board_qualifiers 3 target_variant)

      if("${target_cpucluster}" STREQUAL "cpuapp" AND "${target_variant}" STREQUAL "ns")
        set(board_qualifiers_secure "${target_soc}/cpuapp")
      endif()
    endif()
  elseif(SB_CONFIG_SOC_SERIES_NRF91X)
    list(LENGTH split_board_qualifiers target_length)

    if("${target_length}" GREATER "2")
      list(GET split_board_qualifiers 2 target_variant)
      if("${target_variant}" STREQUAL "ns")
        set(board_qualifiers_secure "${target_soc}")
      endif()
    endif()
  endif()

# TODO: SB_CONFIG_SOC_SERIES_NRF54HX

  if(DEFINED board_qualifiers_secure)
    if(DEFINED BOARD_REVISION)
      set(board_target_secure "${BOARD}@${BOARD_REVISION}/${board_qualifiers_secure}")
    else()
      set(board_target_secure "${BOARD}/${board_qualifiers_secure}")
    endif()
  endif()

  set(split_board_qualifiers)
  set(target_cpucluster)
  set(target_variant)
  set(target_length)

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    if(SB_CONFIG_PARTITION_MANAGER)
      # Make mcuboot a build only target as the main application will flash this from the
      # merged hex file
      set_target_properties(mcuboot PROPERTIES BUILD_ONLY true)

      if(DEFINED board_qualifiers_secure)
        # Apply configuration for MCUboot using all available RAM or not
        if(SB_CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM)
          set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM y)
          set_config_bool(mcuboot CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM y)
          set_config_bool(mcuboot CONFIG_MCUBOOT_NRF_CLEANUP_NONSECURE_RAM y)
        else()
          set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM n)
          set_config_bool(mcuboot CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM n)
        endif()
      endif()
    endif()

    set_property(TARGET mcuboot APPEND_STRING PROPERTY CONFIG "CONFIG_UPDATEABLE_IMAGE_NUMBER=${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES}\n")

    if(SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES)
      set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG "CONFIG_UPDATEABLE_IMAGE_NUMBER=${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES}\n")
    endif()

    # Set MCUboot operating mode in application and MCUboot
# TODO: NCSDK-24488 add RAM LOAD mode
    set(mcuboot_modes "CONFIG_SINGLE_APPLICATION_SLOT;CONFIG_BOOT_SWAP_USING_MOVE;CONFIG_BOOT_SWAP_USING_SCRATCH;CONFIG_BOOT_UPGRADE_ONLY;CONFIG_BOOT_DIRECT_XIP;CONFIG_BOOT_DIRECT_XIP_REVERT;CONFIG_BOOT_FIRMWARE_LOADER")
    set(application_mcuboot_modes "CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP;CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_WITHOUT_SCRATCH;CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH;CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY;CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP;CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT;CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER")

    if(SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
      set(mcuboot_mode CONFIG_SINGLE_APPLICATION_SLOT)
      set(application_mcuboot_mode CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP)
    elseif(SB_CONFIG_MCUBOOT_MODE_SWAP_WITHOUT_SCRATCH)
      set(mcuboot_mode CONFIG_BOOT_SWAP_USING_MOVE)
      set(application_mcuboot_mode CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_WITHOUT_SCRATCH)
    elseif(SB_CONFIG_MCUBOOT_MODE_SWAP_SCRATCH)
      set(mcuboot_mode CONFIG_BOOT_SWAP_USING_SCRATCH)
      set(application_mcuboot_mode CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH)
    elseif(SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY)
      set(mcuboot_mode CONFIG_BOOT_UPGRADE_ONLY)
      set(application_mcuboot_mode CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY)
    elseif(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP)
      set(mcuboot_mode CONFIG_BOOT_DIRECT_XIP)
      set(application_mcuboot_mode CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP)
    elseif(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT)
      set(mcuboot_mode CONFIG_BOOT_DIRECT_XIP;CONFIG_BOOT_DIRECT_XIP_REVERT)
      set(application_mcuboot_mode CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT)
    elseif(SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER)
      set(mcuboot_mode CONFIG_BOOT_FIRMWARE_LOADER)
      set(application_mcuboot_mode CONFIG_MCUBOOT_BOOTLOADER_MODE_FIRMWARE_UPDATER)
    endif()

    # Apply configuration to MCUboot
    foreach(mode ${mcuboot_modes})
      if("${mode}" IN_LIST mcuboot_mode)
        set_config_bool(mcuboot ${mode} y)
      else()
        set_config_bool(mcuboot ${mode} n)
      endif()
    endforeach()

    # Apply configuration to application
    foreach(mode ${application_mcuboot_modes})
      if("${mode}" IN_LIST application_mcuboot_mode)
        set_config_bool(${DEFAULT_IMAGE} ${mode} y)
      else()
        set_config_bool(${DEFAULT_IMAGE} ${mode} n)
      endif()
    endforeach()

    # A v1 board doesn't define board qualifiers, thus below test will just test the pure board
    # name for a v1 board. A v2 board will match against the board qualifier.
    if("${BOARD}${BOARD_QUALIFIERS}" MATCHES "(_|/)ns$")
      # Configure MCUboot before application so that TF-M can read MCUboot configuration
      sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} mcuboot)

      # Configure MCUBoot to be able to boot TFM
      add_overlay_config(
        mcuboot
        ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/tfm.conf
        )

      # Add fault injection protection to MCUBOOT
      add_overlay_config(
        mcuboot
        ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/fih_low_enable.conf
      )
    endif()

    if(SB_CONFIG_PARTITION_MANAGER)
      # Use NCS signing script with support for PM
      if(SB_CONFIG_QSPI_XIP_SPLIT_IMAGE)
        set(${DEFAULT_IMAGE}_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_split.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)

        if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
          set(mcuboot_secondary_app_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_split.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)
        endif()

        # Share QSPI XIP MCUboot image number with the default image to allow for signing script
        # to get the partition manager configuration
        set(${DEFAULT_IMAGE}_QSPI_XIP_IMAGE_NUMBER ${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER} CACHE INTERNAL "QSPI XIP image number" FORCE)

        if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
          set(mcuboot_secondary_app_QSPI_XIP_IMAGE_NUMBER ${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER} CACHE INTERNAL "QSPI XIP image number" FORCE)
        endif()

        # Flash drivers are mandatory with QSPI XIP split image support
        set_config_bool(${DEFAULT_IMAGE} CONFIG_FLASH y)
        set_config_bool(mcuboot CONFIG_FLASH y)
        set_config_bool(mcuboot CONFIG_MULTITHREADING y)
        set_config_bool(mcuboot CONFIG_MCUBOOT_VERIFY_IMG_ADDRESS n)

        # Set options on default image to allow QSPI XIP execution
        set_config_bool(${DEFAULT_IMAGE} CONFIG_XIP y)
        set_config_bool(${DEFAULT_IMAGE} CONFIG_CODE_DATA_RELOCATION y)
        set_config_bool(${DEFAULT_IMAGE} CONFIG_HAVE_CUSTOM_LINKER_SCRIPT y)
        set_config_bool(${DEFAULT_IMAGE} CONFIG_BUILD_NO_GAP_FILL y)

        # Set required options if MCUmgr img mgmt is used
        if(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP)
          set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUMGR_GRP_IMG_QSPI_XIP_SPLIT_IMAGE y)
        else()
          set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUMGR_GRP_IMG_QSPI_XIP_SPLIT_IMAGE n)
        endif()

        if(NOT DEFINED SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK)
          # If external driver check Kconfig is enabled then users will need to select their own
          # Kconfigs (to allow for forked/derivative QSPI NOR drivers), otherwise force enable the
          # driver for the default image and MCUboot
          set_config_bool(${DEFAULT_IMAGE} CONFIG_NORDIC_QSPI_NOR y)
          set_config_bool(${DEFAULT_IMAGE} CONFIG_NORDIC_QSPI_NOR_XIP y)
          set_config_bool(mcuboot CONFIG_NORDIC_QSPI_NOR y)
          set_config_bool(mcuboot CONFIG_NORDIC_QSPI_NOR_XIP y)
        endif()
      else()
        set(${DEFAULT_IMAGE}_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)

        if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
          set(mcuboot_secondary_app_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)
        elseif(SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER)
          set(firmware_loader_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_firmware_loader.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)

          set_target_properties(firmware_loader PROPERTIES
            IMAGE_CONF_SCRIPT ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/image_configurations/firmware_loader_image_default.cmake
          )
        endif()

        set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUMGR_GRP_IMG_QSPI_XIP_SPLIT_IMAGE n)
      endif()

      if(SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK)
        add_overlay_config(mcuboot ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/bl_override/override_external_mcuboot.conf)

        if(NOT SB_CONFIG_MCUBOOT_MODE_SINGLE_APP)
          set_config_bool(${DEFAULT_IMAGE} CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK y)
        endif()
      endif()

      if(SB_CONFIG_PM_EXTERNAL_FLASH_MCUBOOT_SECONDARY)
        add_overlay_config(mcuboot ${ZEPHYR_NRF_MODULE_DIR}/subsys/partition_manager/ext_flash_mcuboot_secondary.conf)
      endif()
    endif()

    if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION y)
      set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS=${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS}\n")
      set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE=${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE}\n")

      set_config_bool(mcuboot CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION y)
      set_config_bool(mcuboot CONFIG_SECURE_BOOT_STORAGE y)
      set_config_bool(mcuboot CONFIG_SECURE_BOOT_CRYPTO y)
    else()
      set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION n)
      set_config_bool(mcuboot CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION n)
    endif()

    if(NOT DEFINED mcuboot_BOARD AND DEFINED board_target_secure)
      # MCUboot must run in secure mode on the nRF9160/nRF5340
      set_target_properties(mcuboot PROPERTIES BOARD ${board_target_secure})
    endif()

    if(SB_CONFIG_MCUBOOT_NRF53_MULTI_IMAGE_UPDATE)
      set_config_bool(mcuboot CONFIG_NRF53_MULTI_IMAGE_UPDATE y)
      set_config_bool(mcuboot CONFIG_BOOT_IMAGE_ACCESS_HOOKS y)
      set_config_bool(mcuboot CONFIG_FLASH_SIMULATOR y)
      set_config_bool(mcuboot CONFIG_FLASH_SIMULATOR_DOUBLE_WRITES y)
      set_config_bool(mcuboot CONFIG_FLASH_SIMULATOR_STATS n)

      if(NOT SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY)
        set_config_bool(mcuboot CONFIG_USE_NRF53_MULTI_IMAGE_WITHOUT_UPGRADE_ONLY y)
      endif()
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT_APPCORE)
    if(NOT DEFINED b0_BOARD AND DEFINED board_target_secure)
      # b0 must run in secure mode on the nRF9160/nRF5340
      set_target_properties(b0 PROPERTIES BOARD ${board_target_secure})

      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        set_target_properties(s1_image PROPERTIES BOARD ${board_target_secure})
      endif()
    endif()

    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      add_overlay_config(mcuboot ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/log_minimal.conf)
      set_config_bool(mcuboot CONFIG_SECURE_BOOT y)
      set_config_bool(mcuboot CONFIG_FW_INFO y)

      if(SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256)
        add_overlay_config(
          mcuboot
          ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/external_crypto.conf
          )
      else()
        message(WARNING "MCUboot and secure boot (application core) are enabled but MCUboot signing key type is not set to ECDSA-P256, this is a non-optimal configuration")
      endif()
    else()
      set_config_bool(${DEFAULT_IMAGE} CONFIG_SECURE_BOOT y)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_FW_INFO y)
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    if(NOT SB_CONFIG_NETCORE_NONE)
      set_config_bool(${SB_CONFIG_NETCORE_IMAGE_NAME} CONFIG_SECURE_BOOT y)
    endif()

    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      if(SB_CONFIG_NETCORE_APP_UPDATE)
        set_config_bool(mcuboot CONFIG_PCD_APP y)

        add_overlay_dts(
          mcuboot
          ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/flash_sim.overlay
          )
        if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
          add_overlay_dts(
            s1_image
            ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/flash_sim.overlay
            )
        endif()
      else()
        set_config_bool(mcuboot CONFIG_PCD_APP n)
      endif()
    endif()
  endif()

  if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_SECURE_BOOT_NETCORE AND SB_CONFIG_BOOTLOADER_MCUBOOT)
    # Disable PCD if there is no secure boot enabled for the network core
    set_config_bool(mcuboot CONFIG_PCD_APP n)
  endif()

  if(SB_CONFIG_BT_FAST_PAIR)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR y)

    if(DEFINED FP_MODEL_ID AND DEFINED FP_ANTI_SPOOFING_KEY)
      include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/fast_pair_hex.cmake)
      set(FP_DATA_PRESENT "y" CACHE INTERNAL "Fast Pair provisioning data provided" FORCE)
    else()
      message(WARNING "Fast Pair support is enabled but `FP_MODEL_ID` or `FP_ANTI_SPOOFING_KEY` were not provided, this is likely to cause a build error")
      set(FP_DATA_PRESENT "n" CACHE INTERNAL "Fast Pair provisioning data provided" FORCE)
    endif()
  else()
    set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR n)
  endif()

  if(SB_CONFIG_WIFI_NRF70)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_WIFI_NRF70 y)

    foreach(config WIFI_PATCHES_EXT_FLASH_DISABLED WIFI_PATCHES_EXT_FLASH_XIP
            WIFI_PATCHES_EXT_FLASH_STORE)
      if(${SB_CONFIG_${config}})
        set_config_bool(${DEFAULT_IMAGE} CONFIG_NRF_${config} ${SB_CONFIG_${config}})
      endif()
    endforeach()

    foreach(config NRF700X_SYSTEM_MODE NRF700X_SCAN_ONLY NRF700X_RADIO_TEST
            NRF700X_SYSTEM_WITH_RAW_MODES)
      if(${SB_CONFIG_WIFI_${config}})
        set_config_bool(${DEFAULT_IMAGE} CONFIG_${config} ${SB_CONFIG_WIFI_${config}})
      endif()
    endforeach()

    if(NOT DEFINED SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_NRF_WIFI_FW_PATCH_DFU n)
    else()
      set_config_bool(${DEFAULT_IMAGE} CONFIG_NRF_WIFI_FW_PATCH_DFU ${SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH})
    endif()
  else()
    set_config_bool(${DEFAULT_IMAGE} CONFIG_WIFI_NRF70 n)
  endif()

  if(SB_CONFIG_ZEPHYR_CONNECTEDHOMEIP_MODULE)
    if(SB_CONFIG_MATTER)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_CHIP y)

      if(SB_CONFIG_MATTER_OTA)
        set_config_bool(${DEFAULT_IMAGE} CONFIG_CHIP_OTA_REQUESTOR y)
        set_config_bool(${DEFAULT_IMAGE} CONFIG_CHIP_OTA_IMAGE_BUILD y)
      else()
        set_config_bool(${DEFAULT_IMAGE} CONFIG_CHIP_OTA_REQUESTOR n)
        set_config_bool(${DEFAULT_IMAGE} CONFIG_CHIP_OTA_IMAGE_BUILD n)
      endif()
    else()
      set_config_bool(${DEFAULT_IMAGE} CONFIG_CHIP n)
    endif()
  endif()

  if(SB_CONFIG_SUIT_BUILD_RECOVERY)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_SUIT_MPI_APP_RECOVERY y)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_SUIT_MPI_RAD_RECOVERY y)
  endif()
endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)

# Sysbuild function hooks used by nRF Connect SDK
function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
  cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})
  restore_ncs_vars()

  set(SYSBUILD y)

  set_property(GLOBAL PROPERTY DOMAIN_APP_APP ${DEFAULT_IMAGE})

  include_packaging()
  include_suit()

  if(SB_CONFIG_SECURE_BOOT OR SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    include_provision_hex()
  endif()

  if(SB_CONFIG_MATTER_FACTORY_DATA_GENERATE AND SB_CONFIG_PARTITION_MANAGER)
    include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/generate_factory_data_sysbuild.cmake)
    nrfconnect_generate_factory_data()
  endif()

  if(SB_CONFIG_MATTER_OTA)
    include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/zephyr/ota-image_sysbuild.cmake)
    if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD)
      chip_ota_image(chip-ota-image
        INPUT_FILES ${CMAKE_BINARY_DIR}/dfu_multi_image.bin
        OUTPUT_FILE ${CMAKE_BINARY_DIR}/${SB_CONFIG_MATTER_OTA_IMAGE_FILE_NAME}
      )
    elseif(SB_CONFIG_SUIT_ENVELOPE)
      chip_ota_image(chip-ota-image
        INPUT_FILES ${CMAKE_BINARY_DIR}/DFU/root.suit
        OUTPUT_FILE ${CMAKE_BINARY_DIR}/${SB_CONFIG_MATTER_OTA_IMAGE_FILE_NAME}
      )
    endif()
  endif()

  if(SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/nrf700x.cmake)
  endif()

  if(SB_CONFIG_DFU_ZIP)
    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/zip.cmake)
    else()
      include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/b0_packaging.cmake)
    endif()
  endif()

  if(SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/mesh_dfu_metadata.cmake)
  endif()

  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/extensions.cmake)
  if(SB_CONFIG_PARTITION_MANAGER)
    # Run partition manager for each image before running CMake.
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/partition_manager.cmake OPTIONAL)

    # Set PM variables up that applications need prior to PM having ran
    get_property(PM_MCUBOOT_PRIMARY_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_SIZE)
    get_property(PM_MCUBOOT_PAD_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PAD_SIZE)

    if(SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP OR SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT)
      get_property(PM_MCUBOOT_PRIMARY_ADDRESS TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_ADDRESS)
      get_property(PM_MCUBOOT_SECONDARY_ADDRESS TARGET partition_manager PROPERTY PM_MCUBOOT_SECONDARY_ADDRESS)
    endif()

    if(SB_CONFIG_MATTER_FACTORY_DATA_GENERATE)
      get_property(PM_FACTORY_DATA_ADDRESS TARGET partition_manager PROPERTY PM_FACTORY_DATA_ADDRESS)
      get_property(PM_FACTORY_DATA_OFFSET TARGET partition_manager PROPERTY PM_FACTORY_DATA_OFFSET)
    endif()

    if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_NETCORE_EMPTY)
      get_property(PM_MCUBOOT_PRIMARY_${SB_CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER}_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_${SB_CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER}_SIZE)
    endif()

    if(SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE)
      get_property(PM_NRF70_WIFI_FW_OFFSET TARGET partition_manager PROPERTY PM_NRF70_WIFI_FW_OFFSET)
      get_property(PM_NRF70_WIFI_FW_SIZE TARGET partition_manager PROPERTY PM_NRF70_WIFI_FW_SIZE)
    endif()

    if(SB_CONFIG_QSPI_XIP_SPLIT_IMAGE)
      get_property(PM_MCUBOOT_PRIMARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS)
      get_property(PM_MCUBOOT_PRIMARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE)
      get_property(PM_MCUBOOT_SECONDARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS TARGET partition_manager PROPERTY PM_MCUBOOT_SECONDARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS)
      get_property(PM_MCUBOOT_SECONDARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_SECONDARY_${SB_CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE)
    elseif(SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER)
      get_property(PM_MCUBOOT_SECONDARY_ADDRESS TARGET partition_manager PROPERTY PM_MCUBOOT_SECONDARY_ADDRESS)
      get_property(PM_MCUBOOT_SECONDARY_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_SECONDARY_SIZE)
    endif()

    # If the network core image is enabled on nRF53, ensure it is flashed before the main application
    if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_NETCORE_NONE)
      sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} ${SB_CONFIG_NETCORE_IMAGE_NAME})
    endif()

    if(SB_CONFIG_QSPI_XIP_SPLIT_IMAGE AND SB_CONFIG_SOC_NRF52840)
      # Emit a warning to the user to not relocate interrupts as this can brick nRF52840 devices
      message(WARNING "
      --------------------------------------------------------------------------------------------
      --- WARNING: SB_CONFIG_QSPI_XIP_SPLIT_IMAGE is enabled and target device is an nRF52840: ---
      --- Care must be taken to not relocate any interrupts to the QSPI XIP flash as this can  ---
      --- cause the SoC to enter an undefined operation state, causing the debug access port   ---
      --- to become blocked, potentially bricking the device.                                  ---
      --------------------------------------------------------------------------------------------
      ")
    endif()
  endif()
  foreach(image ${IMAGES})
    configure_cache(IMAGE ${image})
  endforeach()

  # Add a dependency on the main image's .config file to ensure Kconfig values that have been read
  # and used to generate tasks are not stale, add similar dependencies on other images which have
  # configuration that is read if required options are enabled
  ExternalProject_Get_Property(${DEFAULT_IMAGE} BINARY_DIR)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BINARY_DIR}/zephyr/.config)

  if(NCS_SYSBUILD_PARTITION_MANAGER)
    if(SB_CONFIG_SECURE_BOOT AND SB_CONFIG_SECURE_BOOT_APPCORE AND SB_CONFIG_BOOTLOADER_MCUBOOT)
      ExternalProject_Get_Property(mcuboot BINARY_DIR)
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BINARY_DIR}/zephyr/.config)
    endif()

    if(SB_CONFIG_SECURE_BOOT_NETCORE)
      get_property(main_app GLOBAL PROPERTY DOMAIN_APP_CPUNET)
      ExternalProject_Get_Property(${main_app} BINARY_DIR)
      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BINARY_DIR}/zephyr/.config)
    endif()
  endif()
endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)

# Enable use of partition manager with sysbuild.
# Consider if this shoulc come through Sysbuild Kconfig flag.
set(NCS_SYSBUILD_PARTITION_MANAGER TRUE)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/modules)
include(ncs_sysbuild_extensions)
include(${CMAKE_CURRENT_LIST_DIR}/extensions.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/appcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/netcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/secureboot.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/mcuboot.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/suit.cmake)

store_ncs_vars()
