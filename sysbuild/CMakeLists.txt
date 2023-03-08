#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(include_provision_hex)
  include(${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/cmake/provision_hex.cmake)
endfunction()

function(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)
  cmake_parse_arguments(PRE_CMAKE "" "" "IMAGES" ${ARGN})

  foreach(image ${PRE_CMAKE_IMAGES})
    if(SB_CONFIG_PARTITION_MANAGER)
      set_config_bool(${image} CONFIG_PARTITION_MANAGER_ENABLED y)
    else()
      set_config_bool(${image} CONFIG_PARTITION_MANAGER_ENABLED n)
    endif()
  endforeach()

endfunction(${SYSBUILD_CURRENT_MODULE_NAME}_pre_cmake)

# Sysbuild function hooks used by nRF Connect SDK
function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
  cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})

  set(DOMAIN_APP_APP ${DEFAULT_IMAGE})

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
set(NCS_SYSBUILD_PARTITION_MANAGER TRUE PARENT_SCOPE)

include(${CMAKE_CURRENT_LIST_DIR}/extensions.cmake)

if(SB_CONFIG_SECURE_BOOT)
  set(secure_boot_image_dir ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image)
  if(SB_CONFIG_SECURE_BOOT_NETCORE)
    add_overlay_config(
      ${SB_CONFIG_NETCORE_HCI_RPMSG_NAME} # ToDo: Create a common Kconfig setting
                                          # which can then default to the exact
                                          # remote image selected, as to work for
                                          # hci, mpsl, thread, etc.
      "${secure_boot_image_dir}/secure_boot.conf"
    )

    add_overlay_config(mcuboot "${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd/pcd.conf")
  else()
    if (SB_CONFIG_BOOTLOADER_MCUBOOT)
      add_overlay_config(mcuboot ${secure_boot_image_dir}/multi_image_mcuboot.conf)

      # Include a kconfig file which enables CONFIG_FW_INFO in the image
      # which is booted by B0.
      add_overlay_config(mcuboot ${secure_boot_image_dir}/fw_info.conf)
    endif()
  endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/netcore.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/secureboot.cmake)
