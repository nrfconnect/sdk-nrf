#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

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
  include(${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/cmake/provision_hex.cmake)
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
    set_config_string(${DEFAULT_IMAGE} CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS
                      "--pad-header --header-size ${SB_CONFIG_PM_MCUBOOT_PAD}"
    )

    if(NOT DEFINED mcuboot_BOARD AND DEFINED board_secure)
      # MCUboot must run in secure mode on the nRF9160/nRF5340
        set_target_properties(mcuboot PROPERTIES BOARD ${board_secure})
    endif()
  endif()

  if(SB_CONFIG_SECURE_BOOT AND SB_CONFIG_SECURE_BOOT_APPCORE)
    if(NOT DEFINED b0_BOARD AND DEFINED board_secure)
      # b0 must run in secure mode on the nRF9160/nRF5340
      set_target_properties(b0 PROPERTIES BOARD ${board_secure})

      if(SB_CONFIG_BOOTLOADER_MCUBOOT)
        set_target_properties(s1_image PROPERTIES BOARD ${board_secure})
      endif()
    endif()
  elseif(SB_CONFIG_SECURE_BOOT AND SB_CONFIG_SECURE_BOOT_NETCORE)
    if(SB_CONFIG_NETCORE_APP_UPDATE)
      set_config_bool(mcuboot CONFIG_PCD_APP y)
      set_property(TARGET mcuboot APPEND_STRING PROPERTY CONFIG "CONFIG_UPDATEABLE_IMAGE_NUMBER=2\n")

      add_overlay_config(
        mcuboot
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/partition_manager/ext_flash_mcuboot_secondary.conf
        )
      add_overlay_dts(
        mcuboot
        ${ZEPHYR_NRF_MODULE_DIR}/modules/mcuboot/flash_sim.overlay
        )
    endif()
  endif()
endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)

# Sysbuild function hooks used by nRF Connect SDK
function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
  cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})
  restore_ncs_vars()

  set_property(GLOBAL PROPERTY DOMAIN_APP_APP ${DEFAULT_IMAGE})

  # Run partition manager for each image before running CMake.
  set(domain ${SB_CONFIG_SECURE_BOOT_DOMAIN})
  if(DEFINED domain AND NOT domain STREQUAL APP)
    set(partition_manager_target partition_manager_${domain})
  endif()

  if(SB_CONFIG_SECURE_BOOT)
    include_provision_hex()
  endif()

  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/extensions.cmake)
  if(SB_CONFIG_PARTITION_MANAGER)
    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/partition_manager.cmake OPTIONAL)
  endif()
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
  else()
    if(SB_CONFIG_BOOTLOADER_MCUBOOT)
      add_overlay_config(mcuboot ${secure_boot_image_dir}/multi_image_mcuboot.conf)

      # Include a kconfig file which enables CONFIG_FW_INFO in the image
      # which is booted by B0.
      add_overlay_config(mcuboot ${secure_boot_image_dir}/fw_info.conf)
    endif()
  endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/netcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/secureboot.cmake)

store_ncs_vars()
