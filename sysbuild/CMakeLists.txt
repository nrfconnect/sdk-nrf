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

function(include_fast_pair_hex)
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/fast_pair_hex.cmake)
endfunction()

function(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)
  cmake_parse_arguments(PRE_CMAKE "" "" "IMAGES" ${ARGN})
  restore_ncs_vars()

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

  if(BOARD STREQUAL "nrf9160dk_nrf9160_ns")
    set(board_secure nrf9160dk_nrf9160)
  elseif(BOARD STREQUAL "thingy91_nrf9160_ns")
    set(board_secure thingy91_nrf9160)
  elseif(BOARD STREQUAL "nrf5340dk_nrf5340_cpuapp_ns")
    set(board_secure nrf5340dk_nrf5340_cpuapp)
  elseif(BOARD STREQUAL "thingy53_nrf5340_cpuapp_ns")
    set(board_secure thingy53_nrf5340_cpuapp)
  endif()

  if(DEFINED board_secure AND DEFINED BOARD_REVISION)
    set(board_secure ${board_secure}@${BOARD_REVISION})
  endif()

  if(SB_CONFIG_BOOTLOADER_MCUBOOT)
    # Make mcuboot a build only target as the main application will flash this from the
    # merged hex file
    set_target_properties(mcuboot PROPERTIES BUILD_ONLY true)
    set_property(TARGET mcuboot APPEND_STRING PROPERTY CONFIG "CONFIG_UPDATEABLE_IMAGE_NUMBER=${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES}\n")
    set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG "CONFIG_UPDATEABLE_IMAGE_NUMBER=${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES}\n")

    # Set MCUboot operating mode in application and MCUboot
# TODO: NCSDK-24488 add RAM LOAD mode
    set(mcuboot_modes "CONFIG_SINGLE_APPLICATION_SLOT;CONFIG_BOOT_SWAP_USING_MOVE;CONFIG_BOOT_SWAP_USING_SCRATCH;CONFIG_BOOT_UPGRADE_ONLY;CONFIG_BOOT_DIRECT_XIP;CONFIG_BOOT_DIRECT_XIP_REVERT")
    set(application_mcuboot_modes "CONFIG_MCUBOOT_BOOTLOADER_MODE_SINGLE_APP;CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_WITHOUT_SCRATCH;CONFIG_MCUBOOT_BOOTLOADER_MODE_SWAP_SCRATCH;CONFIG_MCUBOOT_BOOTLOADER_MODE_OVERWRITE_ONLY;CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP;CONFIG_MCUBOOT_BOOTLOADER_MODE_DIRECT_XIP_WITH_REVERT")

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

    # Check if the board is a non-secure one
    string(LENGTH "${BOARD}" board_len)
    math(EXPR board_len "${board_len} - 3")
    string(SUBSTRING "${BOARD}" ${board_len} 3 ns_board_check)

    if("${ns_board_check}" STREQUAL "_ns")
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

    # Use NCS signing script with support for PM
    set(${DEFAULT_IMAGE}_SIGNING_SCRIPT "${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing.cmake" CACHE INTERNAL "MCUboot signing script" FORCE)

    set(imgtool_extra)
    if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
      set(imgtool_extra " --security-counter ${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE}")

      set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG "CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION=${SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION}\n")
      set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS=${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS}\n")
      set_property(TARGET ${DEFAULT_IMAGE} APPEND_STRING PROPERTY CONFIG "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE=${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE}\n")

      add_overlay_config(
        mcuboot
        ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/hw_counters.conf
      )
    endif()

    if(NOT DEFINED mcuboot_BOARD AND DEFINED board_secure)
      # MCUboot must run in secure mode on the nRF9160/nRF5340
      set_target_properties(mcuboot PROPERTIES BOARD ${board_secure})
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT)
    if(SB_CONFIG_SECURE_BOOT_APPCORE)
      if(NOT DEFINED b0_BOARD AND DEFINED board_secure)
        # b0 must run in secure mode on the nRF9160/nRF5340
        set_target_properties(b0 PROPERTIES BOARD ${board_secure})

        if(SB_CONFIG_BOOTLOADER_MCUBOOT)
          set_target_properties(s1_image PROPERTIES BOARD ${board_secure})
        endif()
      endif()

      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        add_overlay_config(
          mcuboot
          ${ZEPHYR_NRF_MODULE_DIR}/sysbuild/mcuboot_enable_secure_bootloader.overlay
          )
        add_overlay_config(
          mcuboot
          ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/external_crypto.conf
          )
      endif()
    endif()

    if(SB_CONFIG_SECURE_BOOT_NETCORE AND SB_CONFIG_BOOTLOADER_MCUBOOT)
      if(SB_CONFIG_NETCORE_APP_UPDATE)
        set_config_bool(mcuboot CONFIG_PCD_APP y)

        add_overlay_config(
          mcuboot
          ${ZEPHYR_NRF_MODULE_DIR}/subsys/partition_manager/ext_flash_mcuboot_secondary.conf
          )
        add_overlay_dts(
          mcuboot
          ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/flash_sim.overlay
          )
      else()
        set_config_bool(mcuboot CONFIG_PCD_APP n)
      endif()
    endif()
  endif()

  if(SB_CONFIG_BT_FAST_PAIR)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR y)

    if(DEFINED FP_MODEL_ID AND DEFINED FP_ANTI_SPOOFING_KEY)
      include_fast_pair_hex()
      set(FP_DATA_PRESENT "y" CACHE INTERNAL "Fast Pair provisioning data provided" FORCE)
    else()
      message(WARNING "Fast Pair support is enabled but `FP_MODEL_ID` or `FP_ANTI_SPOOFING_KEY` were not provided, this is likely to cause a build error")
      set(FP_DATA_PRESENT "n" CACHE INTERNAL "Fast Pair provisioning data provided" FORCE)
    endif()
  else()
    set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR n)
  endif()
endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)

# Sysbuild function hooks used by nRF Connect SDK
function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
  cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})
  restore_ncs_vars()

  set(SYSBUILD y)

  set_property(GLOBAL PROPERTY DOMAIN_APP_APP ${DEFAULT_IMAGE})

  include_packaging()

  if(SB_CONFIG_SECURE_BOOT OR SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
    include_provision_hex()
  endif()

  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/extensions.cmake)
  if(SB_CONFIG_PARTITION_MANAGER)
    # Run partition manager for each image before running CMake.
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/partition_manager.cmake OPTIONAL)
  endif()

  # Set PM variables up that applications need prior to PM having ran
  get_property(PM_MCUBOOT_PRIMARY_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_SIZE)
  get_property(PM_MCUBOOT_PAD_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PAD_SIZE)

  if(SB_CONFIG_SUPPORT_NETCORE AND NOT SB_CONFIG_NETCORE_EMPTY)
    get_property(PM_MCUBOOT_PRIMARY_1_SIZE TARGET partition_manager PROPERTY PM_MCUBOOT_PRIMARY_1_SIZE)
  endif()

  foreach(image ${IMAGES})
    configure_cache(IMAGE ${image})
  endforeach()
endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)

# Enable use of partition manager with sysbuild.
# Consider if this shoulc come through Sysbuild Kconfig flag.
set(NCS_SYSBUILD_PARTITION_MANAGER TRUE)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/modules)
include(ncs_sysbuild_extensions)
include(${CMAKE_CURRENT_LIST_DIR}/extensions.cmake)

if(SB_CONFIG_SECURE_BOOT)
  set(secure_boot_image_dir ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image)
  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    if(SB_CONFIG_NETCORE_HCI_RPMSG)
      add_overlay_config(
        ${SB_CONFIG_NETCORE_HCI_RPMSG_NAME} # ToDo: Create a common Kconfig setting
                                            # which can then default to the exact
                                            # remote image selected, as to work for
                                            # hci, mpsl, thread, etc.
        "${secure_boot_image_dir}/secure_boot.conf"
      )
    elseif(SB_CONFIG_NETCORE_802154_RPMSG)
      add_overlay_config(
        ${SB_CONFIG_NETCORE_802154_RPMSG_NAME} # ToDo: Create a common Kconfig setting
                                            # which can then default to the exact
                                            # remote image selected, as to work for
                                            # hci, mpsl, thread, etc.
        "${secure_boot_image_dir}/secure_boot.conf"
      )
    elseif(SB_CONFIG_NETCORE_EMPTY)
      add_overlay_config(
        ${SB_CONFIG_NETCORE_EMPTY_NAME} # ToDo: Create a common Kconfig setting
                                            # which can then default to the exact
                                            # remote image selected, as to work for
                                            # hci, mpsl, thread, etc.
        "${secure_boot_image_dir}/secure_boot.conf"
      )
    endif()

    add_overlay_config(mcuboot "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf")
  endif()

  if(SB_CONFIG_SECURE_BOOT_APPCORE)
    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      add_overlay_config(mcuboot ${secure_boot_image_dir}/multi_image_mcuboot.conf)

      # Include a kconfig file which enables CONFIG_FW_INFO in the image
      # which is booted by B0.
      add_overlay_config(mcuboot ${secure_boot_image_dir}/fw_info.conf)

      add_overlay_config(mcuboot ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/log_minimal.conf)

      if(SB_CONFIG_PM_OVERRIDE_EXTERNAL_DRIVER_CHECK)
        add_overlay_config(mcuboot ${CMAKE_CURRENT_SOURCE_DIR}/override_external_mcuboot.conf)
      endif()
    else()
      # Include a kconfig file which enables CONFIG_FW_INFO in the image
      # which is booted by B0.
      add_overlay_config(${DEFAULT_IMAGE} ${secure_boot_image_dir}/fw_info.conf)
    endif()
  endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/netcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/secureboot.cmake)

store_ncs_vars()
