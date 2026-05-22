#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_BOOTLOADER_MCUBOOT)
  # Build path to the sysbuild build directory.
  cmake_path(ABSOLUTE_PATH ZEPHYR_BINARY_DIR OUTPUT_VARIABLE app_binary_dir)
  cmake_path(GET app_binary_dir PARENT_PATH app_build_dir)
  cmake_path(GET app_build_dir PARENT_PATH sysbuild_binary_dir)

  if(CONFIG_MCUBOOT_APPLICATION_FIRMWARE_UPDATER)
    cmake_path(APPEND sysbuild_binary_dir "firmware_updater" OUTPUT_VARIABLE output)
  else()
    cmake_path(APPEND sysbuild_binary_dir ${KERNEL_NAME} OUTPUT_VARIABLE output)
  endif()

  if(CONFIG_BUILD_OUTPUT_BIN)
    get_target_property(bin_file runners_yaml_props_target "bin_file")
    if("${bin_file}" STREQUAL "bin_file-NOTFOUND")
      set(bin_file ${KERNEL_BIN_NAME})
    endif()

    if(CONFIG_BUILD_WITH_TFM)
      # TF-M does not generate a bin file, so use the hex file as an input
      set(RUNNERS_BIN_FILE_TO_SIGN "tfm_merged.hex" CACHE STRING "The main output BIN file to sign" FORCE)
    else()
      set(RUNNERS_BIN_FILE_TO_SIGN "${bin_file}" CACHE STRING "The main output BIN file to sign" FORCE)
    endif()
    cmake_path(GET bin_file STEM LAST_ONLY bin_file_name)
  endif()

  if(CONFIG_BUILD_OUTPUT_HEX)
    get_target_property(hex_file runners_yaml_props_target "hex_file")
    if(NOT hex_file)
      set(hex_file ${KERNEL_HEX_NAME})
    endif()

    if(CONFIG_BUILD_WITH_TFM)
      set(RUNNERS_HEX_FILE_TO_SIGN "tfm_merged.hex" CACHE STRING "The main output HEX file to sign" FORCE)
    else()
      set(RUNNERS_HEX_FILE_TO_SIGN "${hex_file}" CACHE STRING "The main output HEX file to sign" FORCE)
    endif()
    cmake_path(GET hex_file STEM LAST_ONLY hex_file_name)
  endif()
endif()
