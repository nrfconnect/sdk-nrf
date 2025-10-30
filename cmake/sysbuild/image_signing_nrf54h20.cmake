#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_BOOTLOADER_MCUBOOT)
  # Build path to the sysbuild build directory.
  cmake_path(ABSOLUTE_PATH ZEPHYR_BINARY_DIR OUTPUT_VARIABLE app_binary_dir)
  cmake_path(GET app_binary_dir PARENT_PATH app_build_dir)
  cmake_path(GET app_build_dir PARENT_PATH sysbuild_binary_dir)
  cmake_path(APPEND sysbuild_binary_dir ${KERNEL_NAME} OUTPUT_VARIABLE output)

  if(CONFIG_BUILD_OUTPUT_BIN)
    get_target_property(bin_file runners_yaml_props_target "bin_file")
    if("${bin_file}" STREQUAL "bin_file-NOTFOUND")
      set(bin_file ${KERNEL_BIN_NAME})
    endif()

    set(RUNNERS_BIN_FILE_TO_MERGE "${bin_file}" CACHE STRING "The main output BIN file to be merged" FORCE)
    if(CONFIG_NCS_IS_VARIANT_IMAGE)
      set_target_properties(runners_yaml_props_target PROPERTIES "bin_file" "${output}_secondary_app.merged.bin")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                   ${CMAKE_COMMAND} "-E" "rm" "-f" "${output}_secondary_app.merged.bin")
    else()
      set_target_properties(runners_yaml_props_target PROPERTIES "bin_file" "${output}.merged.bin")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                   ${CMAKE_COMMAND} "-E" "rm" "-f" "${output}.merged.bin")
    endif()
  endif()

  if(CONFIG_BUILD_OUTPUT_HEX)
    get_target_property(hex_file runners_yaml_props_target "hex_file")
    if("${hex_file}" STREQUAL "hex_file-NOTFOUND")
      set(hex_file ${KERNEL_HEX_NAME})
    endif()

    set(RUNNERS_HEX_FILE_TO_MERGE "${hex_file}" CACHE STRING "The main output HEX file to be merged" FORCE)
    if(CONFIG_NCS_IS_VARIANT_IMAGE)
      set_target_properties(runners_yaml_props_target PROPERTIES "hex_file" "${output}_secondary_app.merged.hex")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                   ${CMAKE_COMMAND} "-E" "rm" "-f" "${output}_secondary_app.merged.hex")
    else()
      set_target_properties(runners_yaml_props_target PROPERTIES "hex_file" "${output}.merged.hex")
      set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                   ${CMAKE_COMMAND} "-E" "rm" "-f" "${output}.merged.hex")
    endif()
  endif()
endif()
