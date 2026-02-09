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

function(include_packaging)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/b0_mcuboot_signing.cmake)
  include(${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/cmake/packaging.cmake)
endfunction()

function(include_hpf)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/hpf.cmake)
endfunction()

function(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)
  cmake_parse_arguments(PRE_CMAKE "" "" "IMAGES" ${ARGN})
  restore_ncs_vars()
  UpdateableImage_Get(updateable_images ALL)

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

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    find_program(IMGTOOL imgtool.py HINTS ${ZEPHYR_MCUBOOT_MODULE_DIR}/scripts/ NAMES imgtool NAMES_PER_DIR)
  endif()

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

  if(SB_CONFIG_SOC_SERIES_NRF53 OR SB_CONFIG_SOC_SERIES_NRF54L)
    list(LENGTH split_board_qualifiers target_length)

    if("${target_length}" GREATER "3")
      list(GET split_board_qualifiers 2 target_cpucluster)
      list(GET split_board_qualifiers 3 target_variant)

      if("${target_cpucluster}" STREQUAL "cpuapp" AND "${target_variant}" STREQUAL "ns")
        set(board_qualifiers_secure "${target_soc}/cpuapp")
      endif()
    endif()
  elseif(SB_CONFIG_SOC_SERIES_NRF91)
    list(LENGTH split_board_qualifiers target_length)

    if("${target_length}" GREATER "2")
      list(GET split_board_qualifiers 2 target_variant)
      if("${target_variant}" STREQUAL "ns")
        set(board_qualifiers_secure "${target_soc}")
      endif()
    endif()
  endif()

# TODO: SB_CONFIG_SOC_SERIES_NRF54H

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
        else()
          set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM n)
          set_config_bool(mcuboot CONFIG_MCUBOOT_USE_ALL_AVAILABLE_RAM n)
        endif()
      endif()
    endif()

    math(EXPR mcuboot_total_images "${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES} + ${SB_CONFIG_MCUBOOT_ADDITIONAL_UPDATEABLE_IMAGES}")
    set_config_int(mcuboot CONFIG_UPDATEABLE_IMAGE_NUMBER ${mcuboot_total_images})

    # Assign image IDs for MCUboot images then set Kconfig in all images with the generated IDs
    set(dfu_slots_sysbuild_kconfigs SB_CONFIG_BOOTLOADER_MCUBOOT;SB_CONFIG_MCUBOOT_NETWORK_CORE_HAS_UPDATE_SLOT;SB_CONFIG_MCUBOOT_WIFI_PATCHES_HAS_UPDATE_SLOT;SB_CONFIG_QSPI_XIP_SPLIT_IMAGE;SB_CONFIG_SECURE_BOOT_APPCORE)
    set(dfu_slots_application_kconfigs CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER;CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER;CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER;CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER;CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER)
    set(dfu_slots_cache_names NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER;NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER;NCS_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER;NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER;NCS_MCUBOOT_MCUBOOT_IMAGE_NUMBER)
    set(dfu_slots_output_names Application;Network;Wifi\ patches;QSPI\ XIP;MCUboot\ b0\ update)
    set(dfu_slots_output_text "-- Sysbuild assigned MCUboot image IDs:\n")
    set(dfu_image_index -1)
    set(manifest_image_target "none")

    list(LENGTH dfu_slots_sysbuild_kconfigs test_things_size)
    math(EXPR test_things_size "${test_things_size} - 1")

    foreach(index RANGE ${test_things_size})
      list(GET dfu_slots_sysbuild_kconfigs ${index} current_sysbuild_kconfig)
      list(GET dfu_slots_application_kconfigs ${index} current_application_kconfig)
      list(GET dfu_slots_cache_names ${index} current_cache_name)
      list(GET dfu_slots_output_names ${index} current_output_name)
      set(value -1)

      if(${current_sysbuild_kconfig})
        math(EXPR dfu_image_index "${dfu_image_index} + 1")
        set(value ${dfu_image_index})
        string(APPEND dfu_slots_output_text "   * ${current_output_name}: ${value}\n")
      endif()

      foreach(image mcuboot;${updateable_images})
        set_config_int(${image} ${current_application_kconfig} ${value})
      endforeach()

      if(SB_CONFIG_MCUBOOT_MANIFEST_UPDATES)
        if(${value} EQUAL ${SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX})
          if("${current_application_kconfig}" STREQUAL "CONFIG_MCUBOOT_APPLICATION_IMAGE_NUMBER")
            set(manifest_image_target ${DEFAULT_IMAGE})
          elseif("${current_application_kconfig}" STREQUAL "CONFIG_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER")
            set(manifest_image_target ${SB_CONFIG_NETCORE_IMAGE_NAME})
          else()
            # Unsupported indexes:
            # - CONFIG_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER
            # - CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER
            # - CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER
            message(FATAL_ERROR "MCUboot manifest image can only be assigned to application or network core images")
          endif()
        endif()
      endif()

      set(${current_cache_name} ${value} CACHE INTERNAL "" FORCE)
    endforeach()

    # Assign image IDs for MCUboot extra images
    set(dfu_slots_application_kconfigs CONFIG_MCUBOOT_EXTRA_1_IMAGE_NUMBER;CONFIG_MCUBOOT_EXTRA_2_IMAGE_NUMBER)
    set(dfu_slots_cache_names NCS_MCUBOOT_EXTRA_1_IMAGE_NUMBER;NCS_MCUBOOT_EXTRA_2_IMAGE_NUMBER)
    set(dfu_slots_output_names "Extra Image 1;Extra Image 2")

    if(SB_CONFIG_MCUBOOT_EXTRA_IMAGES GREATER 0)
      math(EXPR extra_images_last_index "${SB_CONFIG_MCUBOOT_EXTRA_IMAGES} - 1")
      foreach(index RANGE ${extra_images_last_index})
        list(GET dfu_slots_application_kconfigs ${index} current_application_kconfig)
        list(GET dfu_slots_cache_names ${index} current_cache_name)
        list(GET dfu_slots_output_names ${index} current_output_name)
        set(value -1)

        if(SB_CONFIG_MCUBOOT_EXTRA_IMAGES GREATER index)
          math(EXPR dfu_image_index "${dfu_image_index} + 1")
          set(value ${dfu_image_index})
          string(APPEND dfu_slots_output_text "   * ${current_output_name}: ${value}\n")
        endif()

        foreach(image mcuboot;${updateable_images})
          set_config_int(${image} ${current_application_kconfig} ${value})
        endforeach()

        set(${current_cache_name} ${value} CACHE INTERNAL "" FORCE)
      endforeach()
    endif()

    # Output details on image IDs
    string(LENGTH ${dfu_slots_output_text} dfu_slots_output_text_length)
    math(EXPR dfu_slots_output_text_length "${dfu_slots_output_text_length} - 1")
    string(SUBSTRING ${dfu_slots_output_text} 0 ${dfu_slots_output_text_length} dfu_slots_output_text)
    message(NOTICE ${dfu_slots_output_text})

    set(dfu_slots_sysbuild_kconfigs)
    set(dfu_slots_application_kconfigs)
    set(dfu_slots_cache_names)
    set(dfu_slots_output_names)
    set(dfu_slots_output_text)
    set(dfu_slots_output_text_length)
    set(dfu_image_index)
    set(value)
    set(current_sysbuild_kconfig)
    set(current_application_kconfig)
    set(current_cache_name)
    set(current_output_name)

    if(SB_CONFIG_MCUBOOT_APP_SYNC_UPDATEABLE_IMAGES)
      foreach(image ${updateable_images})
        set_config_int(${image} CONFIG_UPDATEABLE_IMAGE_NUMBER ${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES})
      endforeach()
    endif()

    if(SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE)
      set_config_bool(mcuboot CONFIG_BOOT_SIGNATURE_TYPE_PURE y)
    endif()

    if(SB_CONFIG_BOOT_IMG_HASH_ALG_SHA512 AND NOT (SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU AND SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE))
      set_config_bool(mcuboot CONFIG_BOOT_IMG_HASH_ALG_SHA512 y)
    endif()

    if(SB_CONFIG_MCUBOOT_MANIFEST_UPDATES)
      set_config_bool(mcuboot CONFIG_MCUBOOT_MANIFEST_UPDATES y)
      set_config_int(mcuboot CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX
        ${SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX})

      if("${manifest_image_target}" STREQUAL "none")
        message(FATAL_ERROR "No manifest image target, cannot append manifest to image")
      endif()
      set_config_bool(${manifest_image_target} CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST y)
      if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
        if("${manifest_image_target}" STREQUAL "${DEFAULT_IMAGE}")
          set_config_bool(mcuboot_secondary_app CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST y)
        else()
          set_config_bool(${manifest_image_target}_secondary_app
            CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST y)
        endif()
      endif()
    endif()

    # Apply configuration to application
    foreach(image ${updateable_images})
      foreach(mode ${application_mcuboot_modes})
        if("${mode}" IN_LIST application_mcuboot_mode)
          set_config_bool(${image} ${mode} y)
        else()
          set_config_bool(${image} ${mode} n)
        endif()
      endforeach()

      if(SB_CONFIG_MCUBOOT_MANIFEST_UPDATES)
        set_config_bool(${image} CONFIG_NCS_MCUBOOT_MANIFEST_UPDATES y)
        set_config_int(${image} CONFIG_NCS_MCUBOOT_MANIFEST_IMAGE_INDEX ${SB_CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX})
      endif()

      if(SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
        set_config_bool(${image} CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_ED25519 y)
      endif()

      if(SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE)
        set_config_bool(${image} CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE y)
      endif()

      if(SB_CONFIG_BOOT_IMG_HASH_ALG_SHA512)
        set_config_bool(${image} CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512 y)
      endif()
    endforeach()

    if(SB_CONFIG_MCUBOOT_MODE_FIRMWARE_UPDATER)
      FirmwareUpdaterImage_Get(fw_loader_images)
      foreach(image ${fw_loader_images})
        if(SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
          set_config_bool(${image} CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_ED25519 y)

          if(SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE)
            set_config_bool(${image} CONFIG_MCUBOOT_BOOTLOADER_SIGNATURE_TYPE_PURE y)
          endif()

          if(SB_CONFIG_BOOT_IMG_HASH_ALG_SHA512)
            set_config_bool(${image} CONFIG_MCUBOOT_BOOTLOADER_USES_SHA512 y)
          endif()
        endif()

        if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY AND SB_CONFIG_SOC_NRF54H20)
          set(${image}_SIGNING_SCRIPT
            "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_nrf54h20.cmake" CACHE INTERNAL
            "MCUboot signing script" FORCE)
        else()
          set(${image}_SIGNING_SCRIPT
            "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_firmware_loader.cmake" CACHE
            INTERNAL "MCUboot signing script" FORCE)
        endif()
        set_target_properties(${image} PROPERTIES IMAGE_CONF_SCRIPT
          ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/image_configurations/firmware_loader_image_default.cmake
        )

        if(NOT ${image} STREQUAL ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME})
          add_dependencies(${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME} ${image})
          sysbuild_add_dependencies(FLASH ${SB_CONFIG_FIRMWARE_LOADER_IMAGE_NAME} ${image})
        endif()

        if(SB_CONFIG_FIRMWARE_LOADER_RADIO AND NOT SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NONE)
          if(NOT ${image} STREQUAL ${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NAME})
            add_dependencies(${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NAME} ${image})
            sysbuild_add_dependencies(FLASH ${SB_CONFIG_FIRMWARE_LOADER_RADIO_IMAGE_NAME} ${image})
          endif()
        endif()
      endforeach()
    endif()

    # The NRF54LX goes with PSA crypto by default
    if(SB_CONFIG_SOC_SERIES_NRF54L)
      if(SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE)
        set_config_bool(mcuboot CONFIG_NRF_SECURITY y)
      elseif(SB_CONFIG_BOOT_SIGNATURE_TYPE_ED25519)
        set_config_bool(mcuboot CONFIG_NRF_SECURITY y)

        # We are sure that ED25519 signature on MCUboot does not need these
        set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_AEAD_DRIVER n)
        set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_PAKE_DRIVER n)
        if(SB_CONFIG_BOOT_ENCRYPTION)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_CIPHER_DRIVER y)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_MAC_DRIVER y)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_KEY_AGREEMENT_DRIVER y)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_KEY_DERIVATION_DRIVER y)
        else()
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_CIPHER_DRIVER n)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_MAC_DRIVER n)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_KEY_AGREEMENT_DRIVER n)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_KEY_DERIVATION_DRIVER n)
        endif()

        if(SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU)
          set_config_bool(mcuboot CONFIG_BOOT_SIGNATURE_USING_KMU y)

          if(SB_CONFIG_MCUBOOT_SIGNATURE_KMU_UROT_MAPPING)
            set_config_bool(mcuboot CONFIG_NCS_BOOT_SIGNATURE_KMU_UROT_MAPPING y)
          else()
            set_config_bool(mcuboot CONFIG_NCS_BOOT_SIGNATURE_KMU_UROT_MAPPING n)
          endif()
        else()
          set_config_bool(mcuboot CONFIG_BOOT_SIGNATURE_USING_KMU n)
        endif()

        # MCUboot uses hash function to identify key internally when KMU is disabled.
        if(SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU AND SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE)
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_HASH_DRIVER n)
        else()
          set_config_bool(mcuboot CONFIG_PSA_USE_CRACEN_HASH_DRIVER y)
        endif()
      endif()
    endif()

    if(SB_CONFIG_SOC_SERIES_NRF54H)
      if(SB_CONFIG_MCUBOOT_SIGNATURE_USING_ITS)
        set_config_bool(mcuboot CONFIG_NCS_BOOT_SIGNATURE_USING_ITS y)
      else()
        set_config_bool(mcuboot CONFIG_NCS_BOOT_SIGNATURE_USING_ITS n)
      endif()
    endif()

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
      set_config_bool(mcuboot CONFIG_BOOT_FIH_PROFILE_DEFAULT_LOW y)
    endif()

    if(SB_CONFIG_PARTITION_MANAGER OR SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP
       OR SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT
       OR SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT
       OR (SB_CONFIG_SOC_SERIES_NRF54L AND SB_CONFIG_BOOT_ENCRYPTION)
       OR SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION
       OR SB_CONFIG_MCUBOOT_MANIFEST_UPDATES
       # Do not use the upstream mcuboot.cmake on nRF54H.
       OR SB_CONFIG_SOC_SERIES_NRF54H)
      # Use NCS signing script with support for PM or direct XIP (NCS specific features)
      if(SB_CONFIG_QSPI_XIP_SPLIT_IMAGE)
        set(${DEFAULT_IMAGE}_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_split.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)

        if(SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
          set(mcuboot_secondary_app_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_split.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)
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
          set_config_bool(${DEFAULT_IMAGE} CONFIG_MCUMGR_GRP_IMG_NRF y)
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
        foreach(image ${updateable_images})
          set(${image}_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)
          set_config_bool(${image} CONFIG_MCUMGR_GRP_IMG_QSPI_XIP_SPLIT_IMAGE n)
        endforeach()
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

      if(SB_CONFIG_SECURE_BOOT_APPCORE)
        # Get the s0/s1 MCUboot update package version and split it up into the fields so it can
        # be supplied to the MCUboot image
        string(REPLACE "." ";" s0_s1_package_version ${SB_CONFIG_SECURE_BOOT_MCUBOOT_VERSION})
        string(REPLACE "+" ";" s0_s1_package_version "${s0_s1_package_version}")

        list(GET s0_s1_package_version 0 s0_s1_package_version_major)
        list(GET s0_s1_package_version 1 s0_s1_package_version_minor)
        list(GET s0_s1_package_version 2 s0_s1_package_version_revision)
        list(GET s0_s1_package_version 3 s0_s1_package_version_build_number)

        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_MAJOR ${s0_s1_package_version_major})
        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_MINOR ${s0_s1_package_version_minor})
        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_REVISION ${s0_s1_package_version_revision})
        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_BUILD_NUMBER ${s0_s1_package_version_build_number})

        set(s0_s1_package_version)
        set(s0_s1_package_version_major)
        set(s0_s1_package_version_minor)
        set(s0_s1_package_version_revision)
        set(s0_s1_package_version_build_number)
      else()
        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_MAJOR -1)
        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_MINOR -1)
        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_REVISION -1)
        set_config_int(mcuboot CONFIG_MCUBOOT_MCUBOOT_S0_S1_VERSION_BUILD_NUMBER -1)
      endif()
    endif()

    if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
      set_config_bool(mcuboot CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION y)

      # nRF54H20 uses SDFW-based counters.
      # There is no need for a dedicated secure boot storage implementation.
      if(NOT SB_CONFIG_SOC_SERIES_NRF54H)
        set_config_bool(mcuboot CONFIG_SECURE_BOOT_STORAGE y)
        set_config_bool(mcuboot CONFIG_SECURE_BOOT_CRYPTO y)
      endif()
    else()
      set_config_bool(mcuboot CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION n)
    endif()

    foreach(image ${updateable_images})
      if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
        set_config_bool(${image} CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION y)
        set_config_int(${image} CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE ${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE})

        # The number of slots is unlimited in the current SDFW-based implementation.
        if(SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS)
          set_config_int(${image} CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS ${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS})
        endif()
      else()
        set_config_bool(${image} CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION n)
      endif()
    endforeach()

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

    # Apply image compression support options
    if(SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT)
      set_config_bool(mcuboot CONFIG_NRF_COMPRESS y)
      set_config_bool(mcuboot CONFIG_NRF_COMPRESS_DECOMPRESSION y)
      set_config_bool(mcuboot CONFIG_NRF_COMPRESS_LZMA y)
      set_config_bool(mcuboot CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2 y)
      set_config_bool(mcuboot CONFIG_NRF_COMPRESS_ARM_THUMB y)
      set_config_bool(mcuboot CONFIG_NRF_COMPRESS_CLEANUP y)
      set_config_bool(mcuboot CONFIG_BOOT_DECOMPRESSION y)
    else()
      set_config_bool(mcuboot CONFIG_BOOT_DECOMPRESSION n)
    endif()

    foreach(image ${updateable_images})
      if(SB_CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT)
        set_config_bool(${image} CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT_ENABLED y)
      else()
        set_config_bool(${image} CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT_ENABLED n)
      endif()
    endforeach()
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
        if(SB_CONFIG_BOOT_SHARED_CRYPTO_ECDSA_P256)
          add_overlay_config(
            mcuboot
            ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/external_crypto.conf
            )
        endif()
      endif()

      if((NOT SB_CONFIG_SECURE_BOOT_SIGNATURE_TYPE_ECDSA AND SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256) OR (SB_CONFIG_SECURE_BOOT_SIGNATURE_TYPE_ECDSA AND NOT SB_CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256))
        message(WARNING "MCUboot and secure boot (application core) are enabled but MCUboot signing key type is not set to ECDSA-P256, this is a non-optimal configuration if sharing of crypto functions is needed from b0 to MCUboot")
      endif()
    else()
      set_config_bool(${DEFAULT_IMAGE} CONFIG_SECURE_BOOT y)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_FW_INFO y)
    endif()

    if(SB_CONFIG_SECURE_BOOT_MONOTONIC_COUNTER)
      set_config_bool(b0 CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION y)

      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        set_config_bool(mcuboot CONFIG_SB_MONOTONIC_COUNTER_IS_ENABLED y)
      else()
        set_config_bool(${DEFAULT_IMAGE} CONFIG_SB_MONOTONIC_COUNTER_IS_ENABLED y)
      endif()

      if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
        set_config_bool(s1_image CONFIG_SB_MONOTONIC_COUNTER_IS_ENABLED y)
      endif()
    else()
      set_config_bool(b0 CONFIG_SB_MONOTONIC_COUNTER_ROLLBACK_PROTECTION n)

      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        set_config_bool(mcuboot CONFIG_SB_MONOTONIC_COUNTER_IS_ENABLED n)
      else()
        set_config_bool(${DEFAULT_IMAGE} CONFIG_SB_MONOTONIC_COUNTER_IS_ENABLED n)
      endif()

      if(SB_CONFIG_SECURE_BOOT_BUILD_S1_VARIANT_IMAGE)
        set_config_bool(s1_image CONFIG_SB_MONOTONIC_COUNTER_IS_ENABLED n)
      endif()
    endif()

    # Apply configuration for hashing
    if(SB_CONFIG_SECURE_BOOT_HASH_TYPE_SHA256 AND NOT SB_CONFIG_SECURE_BOOT_APPCORE_HASH_TYPE_NONE)
      if(SB_CONFIG_SECURE_BOOT_APPCORE_HASH_TYPE_HARDWARE)
        set_config_bool(b0 CONFIG_SB_CRYPTO_CC310_SHA256 y)
      elseif(SB_CONFIG_SECURE_BOOT_APPCORE_HASH_TYPE_SOFTWARE)
        set_config_bool(b0 CONFIG_SB_CRYPTO_OBERON_SHA256 y)
      elseif(SB_CONFIG_SECURE_BOOT_APPCORE_HASH_TYPE_UNUSED)
        set_config_bool(b0 CONFIG_SB_CRYPTO_NO_SHA256 y)
      endif()
    elseif(SB_CONFIG_SECURE_BOOT_HASH_TYPE_NONE OR SB_CONFIG_SECURE_BOOT_APPCORE_HASH_TYPE_NONE)
      set_config_bool(b0 CONFIG_SB_CRYPTO_NONE y)
    endif()

    if(SB_CONFIG_SECURE_BOOT_SIGNATURE_TYPE_ED25519)
      if(SB_CONFIG_SECURE_BOOT_APPCORE_SIGNATURE_TYPE_HARDWARE)
        set_config_bool(b0 CONFIG_NRF_SECURITY y)
        set_config_bool(b0 CONFIG_SB_CRYPTO_PSA_ED25519 y)
      endif()
    elseif(SB_CONFIG_SECURE_BOOT_SIGNATURE_TYPE_ECDSA)
      if(SB_CONFIG_SECURE_BOOT_APPCORE_SIGNATURE_TYPE_HARDWARE)
        set_config_bool(b0 CONFIG_SB_CRYPTO_CC310_ECDSA_SECP256R1 y)
      elseif(SB_CONFIG_SECURE_BOOT_APPCORE_SIGNATURE_TYPE_SOFTWARE)
        set_config_bool(b0 CONFIG_SB_CRYPTO_OBERON_ECDSA_SECP256R1 y)
      elseif(SB_CONFIG_SECURE_BOOT_APPCORE_SIGNATURE_TYPE_UNUSED)
        set_config_bool(b0 CONFIG_SB_CRYPTO_NO_ECDSA_SECP256R1 y)
      endif()
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES)
    foreach(image ${PRE_CMAKE_IMAGES})
      set_config_bool(${image} CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS n)
    endforeach()
  endif()

  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    if(NOT SB_CONFIG_NETCORE_NONE)
      set_config_bool(${SB_CONFIG_NETCORE_IMAGE_NAME} CONFIG_SECURE_BOOT y)
    endif()

    # Apply configuration for hashing
    if(SB_CONFIG_SECURE_BOOT_HASH_TYPE_SHA256 AND NOT SB_CONFIG_SECURE_BOOT_NETCORE_HASH_TYPE_NONE)
      if(SB_CONFIG_SECURE_BOOT_NETCORE_HASH_TYPE_SOFTWARE)
        set_config_bool(b0n CONFIG_SB_CRYPTO_OBERON_SHA256 y)
      elseif(SB_CONFIG_SECURE_BOOT_NETCORE_HASH_TYPE_UNUSED)
        set_config_bool(b0n CONFIG_SB_CRYPTO_NO_SHA256 y)
      endif()
    elseif(SB_CONFIG_SECURE_BOOT_HASH_TYPE_NONE OR SB_CONFIG_SECURE_BOOT_NETCORE_HASH_TYPE_NONE)
      set_config_bool(b0n CONFIG_SB_CRYPTO_NONE y)
    endif()

    if(SB_CONFIG_SECURE_BOOT_SIGNATURE_TYPE_ECDSA)
      if(SB_CONFIG_SECURE_BOOT_NETCORE_SIGNATURE_TYPE_SOFTWARE)
        set_config_bool(b0n CONFIG_SB_CRYPTO_OBERON_ECDSA_SECP256R1 y)
      elseif(SB_CONFIG_SECURE_BOOT_NETCORE_SIGNATURE_TYPE_UNUSED)
        set_config_bool(b0n CONFIG_SB_CRYPTO_NO_ECDSA_SECP256R1 y)
      endif()
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

  if(SB_CONFIG_BT_FAST_PAIR_PROV_DATA AND SB_CONFIG_PARTITION_MANAGER)
    # The PM properties for the Fast Pair hex file must be set in the pre-CMake stage.
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/fast_pair/hex.cmake)
  endif()

  if(SB_CONFIG_WIFI_NRF70)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_WIFI_NRF70 y)

    foreach(config WIFI_PATCHES_EXT_FLASH_DISABLED WIFI_PATCHES_EXT_FLASH_XIP
            WIFI_PATCHES_EXT_FLASH_STORE)
      if(${SB_CONFIG_${config}})
        set_config_bool(${DEFAULT_IMAGE} CONFIG_NRF_${config} ${SB_CONFIG_${config}})
      endif()
    endforeach()

    if(NOT DEFINED SB_CONFIG_WIFI_PATCHES_EXT_FLASH_DISABLED)
      set_config_bool(${DEFAULT_IMAGE} CONFIG_NRF_WIFI_PATCHES_BUILTIN n)
    endif()

    foreach(config NRF70_SYSTEM_MODE NRF70_SCAN_ONLY NRF70_RADIO_TEST
            NRF70_SYSTEM_WITH_RAW_MODES)
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

  if(SB_CONFIG_HPF)
    include_hpf()
  endif()

  if(SB_CONFIG_APPROTECT_USE_UICR OR SB_CONFIG_APPROTECT_LOCK OR SB_CONFIG_APPROTECT_USER_HANDLING OR SB_CONFIG_APPROTECT_DISABLE OR
     SB_CONFIG_SECURE_APPROTECT_USE_UICR OR SB_CONFIG_SECURE_APPROTECT_LOCK OR SB_CONFIG_SECURE_APPROTECT_USER_HANDLING OR SB_CONFIG_SECURE_APPROTECT_DISABLE)
    # Apply APPROTECT configuration to target images
    if(SB_CONFIG_APPROTECT_USE_UICR)
      set(normal_approtect CONFIG_NRF_APPROTECT_USE_UICR)
    elseif(SB_CONFIG_APPROTECT_LOCK)
      set(normal_approtect CONFIG_NRF_APPROTECT_LOCK)
    elseif(SB_CONFIG_APPROTECT_USER_HANDLING)
      set(normal_approtect CONFIG_NRF_APPROTECT_USER_HANDLING)
    elseif(SB_CONFIG_APPROTECT_DISABLE)
      set(normal_approtect CONFIG_NRF_APPROTECT_DISABLE)
    endif()

    if(SB_CONFIG_SECURE_APPROTECT_USE_UICR)
      set(secure_approtect CONFIG_NRF_SECURE_APPROTECT_USE_UICR)
    elseif(SB_CONFIG_SECURE_APPROTECT_LOCK)
      set(secure_approtect CONFIG_NRF_SECURE_APPROTECT_LOCK)
    elseif(SB_CONFIG_SECURE_APPROTECT_USER_HANDLING)
      set(secure_approtect CONFIG_NRF_SECURE_APPROTECT_USER_HANDLING)
    elseif(SB_CONFIG_SECURE_APPROTECT_DISABLE)
      set(secure_approtect CONFIG_NRF_SECURE_APPROTECT_DISABLE)
    endif()

    if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_NETCORE_NONE)
      get_property(network_images GLOBAL PROPERTY PM_${SB_CONFIG_NETCORE_IMAGE_DOMAIN}_IMAGES)
    endif()

    foreach(image ${IMAGES})
      if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_NETCORE_NONE AND ${image} IN_LIST network_images)
        # Network core image
        if(normal_approtect)
          set_config_bool(${image} ${normal_approtect} y)
        endif()
      else()
        # Application (or other) core image
        if(normal_approtect)
          set_config_bool(${image} ${normal_approtect} y)
        endif()
        if(secure_approtect)
          set_config_bool(${image} ${secure_approtect} y)
        endif()
      endif()
    endforeach()

    set(normal_approtect)
    set(secure_approtect)
    set(network_images)
  endif()

  if(SB_CONFIG_CRACEN_MICROCODE_LOAD_ONCE)
    if(SB_CONFIG_CRACEN_MICROCODE_LOAD_B0)
      set_config_bool(b0 CONFIG_CRACEN_LOAD_MICROCODE y)

      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        set_config_bool(mcuboot CONFIG_CRACEN_LOAD_MICROCODE n)
      endif()
    elseif(SB_CONFIG_CRACEN_MICROCODE_LOAD_MCUBOOT)
      if(SB_CONFIG_SECURE_BOOT_APPCORE)
        set_config_bool(b0 CONFIG_CRACEN_LOAD_MICROCODE n)
      endif()

      set_config_bool(mcuboot CONFIG_CRACEN_LOAD_MICROCODE y)
    endif()

    set_config_bool(${DEFAULT_IMAGE} CONFIG_CRACEN_LOAD_MICROCODE n)
  endif()

  if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY AND SB_CONFIG_SOC_NRF54H20)
    UpdateableImage_Get(images ALL)
    foreach(image ${images})
      set_config_bool(${image} CONFIG_NCS_MCUBOOT_BOOTLOADER_SIGN_MERGED_BINARY true)
      set(${image}_SIGNING_SCRIPT
        "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_nrf54h20.cmake"
         CACHE INTERNAL "MCUboot signing script" FORCE)
    endforeach()

    FirmwareUpdaterImage_Get(images ALL)
    foreach(image ${images})
      set_config_bool(${image} CONFIG_NCS_MCUBOOT_BOOTLOADER_SIGN_MERGED_BINARY true)
      set(${image}_SIGNING_SCRIPT
        "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_nrf54h20.cmake"
         CACHE INTERNAL "MCUboot signing script" FORCE)
    endforeach()
  endif()
endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)

function(${SYSBUILD_CURRENT_MODULE_NAME}_pre_image_cmake)
  cmake_parse_arguments(PRE_IMAGE_CMAKE "" "IMAGE" "" ${ARGN})

  get_property(variant_app TARGET ${PRE_IMAGE_CMAKE_IMAGE} PROPERTY NCS_VARIANT_APPLICATION)
  if(NOT "${variant_app}" STREQUAL "")
    set(variant_image ${PRE_IMAGE_CMAKE_IMAGE})

    get_cmake_property(sysbuild_cache CACHE_VARIABLES)
    foreach(var_name ${sysbuild_cache})
      if("${var_name}" MATCHES "^${variant_app}_(.*)$")
        set(${variant_image}_${CMAKE_MATCH_1} "${${var_name}}" CACHE UNINITIALIZED "" FORCE)
      endif()
    endforeach()
  endif()
endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_pre_image_cmake)

# Sysbuild function hooks used by nRF Connect SDK
function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
  cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})
  restore_ncs_vars()

  set(SYSBUILD y)

  set_property(GLOBAL PROPERTY DOMAIN_APP_APP ${DEFAULT_IMAGE})

  # Include any files that need to merge files with uicr_merged.hex
  if(SB_CONFIG_BT_FAST_PAIR_PROV_DATA AND NOT SB_CONFIG_PARTITION_MANAGER)
    # The DTS partition for the Fast Pair hex file is available in the post-CMake stage.
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/fast_pair/hex.cmake)
  endif()

  if(SB_CONFIG_MATTER_FACTORY_DATA_GENERATE)
    include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/chip-module/generate_factory_data_sysbuild.cmake)
    nrfconnect_generate_factory_data()
  endif()

  # Sign extra DFU images added via dfu_extra_add_binary()
  if(SB_CONFIG_MCUBOOT_EXTRA_IMAGES)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing_extra.cmake)
  endif()

  include_packaging()

  # nRF54H20 uses SDFW-based counters.
  # There is no need to generate a provisioning hex file.
  if(SB_CONFIG_SECURE_BOOT OR (SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION AND NOT
     SB_CONFIG_SOC_SERIES_NRF54H))
    include_provision_hex()
  endif()

  if(SB_CONFIG_SECURE_BOOT_GENERATE_DEFAULT_KMU_KEYFILE OR SB_CONFIG_MCUBOOT_GENERATE_DEFAULT_KMU_KEYFILE)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/generate_default_keyfile.cmake)
  endif()

  if(SB_CONFIG_MATTER_OTA)
    include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/zephyr/ota-image_sysbuild.cmake)
    if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_BUILD)
      chip_ota_image(chip-ota-image
        INPUT_FILES ${CMAKE_BINARY_DIR}/dfu_multi_image.bin
        OUTPUT_FILE ${CMAKE_BINARY_DIR}/${SB_CONFIG_MATTER_OTA_IMAGE_FILE_NAME}
      )
    endif()
  endif()

  if(SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/nrf700x.cmake)
  endif()

  if(SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootconf.cmake)
  endif()

  if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY AND SB_CONFIG_SOC_NRF54H20)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/mcuboot_nrf54h20.cmake)
  endif()

  if(SB_CONFIG_BOOTLOADER_MCUBOOT AND SB_CONFIG_MCUBOOT_MANIFEST_UPDATES)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/mcuboot_manifest.cmake)
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

  if(SB_CONFIG_LWM2M_CARRIER_DIVIDED_DFU)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/lwm2m_carrier_divided_dfu.cmake)
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
      get_property(PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER}_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER}_SIZE)
    endif()

    if(SB_CONFIG_WIFI_PATCHES_EXT_FLASH_STORE)
      get_property(PM_NRF70_WIFI_FW_OFFSET TARGET partition_manager PROPERTY PM_NRF70_WIFI_FW_OFFSET)
      get_property(PM_NRF70_WIFI_FW_SIZE TARGET partition_manager PROPERTY PM_NRF70_WIFI_FW_SIZE)
    endif()

    if(SB_CONFIG_QSPI_XIP_SPLIT_IMAGE)
      get_property(PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS)
      get_property(PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE)
      get_property(PM_MCUBOOT_SECONDARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS TARGET partition_manager PROPERTY PM_MCUBOOT_SECONDARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_ADDRESS)
      get_property(PM_MCUBOOT_SECONDARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_SECONDARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_SIZE)
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

  if(SB_CONFIG_SOC_NRF54LS05B AND SB_CONFIG_PARTITION_MANAGER)
    message(WARNING "nrf54ls05 needs to move away from partition manager, usage of PM is temporary")
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
include(${CMAKE_CURRENT_LIST_DIR}/flprcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/pprcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/radio_loader.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/secureboot.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/mcuboot.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/firmware_loader_radio.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/nrf70.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dfu_extra_images.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/bt_fast_pair.cmake)

store_ncs_vars()
