#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file is responsible for triggering the generation of the signed image
# whenever the source manifest YAML is updated.

if("${CONFIG_NCS_MCUBOOT_MANIFEST_DIR}" STREQUAL "")
  set(manifest_dir "${CMAKE_BINARY_DIR}")
else()
  set(manifest_dir "${CONFIG_NCS_MCUBOOT_MANIFEST_DIR}")
endif()

if(CONFIG_NCS_IS_VARIANT_IMAGE)
  cmake_path(APPEND manifest_dir "manifest_secondary.yaml" OUTPUT_VARIABLE manifest_path)
else()
  cmake_path(APPEND manifest_dir "manifest.yaml" OUTPUT_VARIABLE manifest_path)
endif()

if(CONFIG_NCS_MCUBOOT_IMGTOOL_APPEND_MANIFEST)
  # The manifest image should trigger the post build commands whenever a manifest
  # is updated to rerun the manifest generation.
  zephyr_library()
  zephyr_library_sources(${CMAKE_CURRENT_BINARY_DIR}/empty.c)

  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/empty.c"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/empty.c"
    DEPENDS "${manifest_path}"
  )
  add_custom_target(empty_c DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/empty.c")
  add_dependencies(${ZEPHYR_CURRENT_LIBRARY} empty_c)
elseif(NOT CONFIG_MCUBOOT)
  # Other images should trigger a manifest change whenever there is a need
  # to rebuild them and as a result, the image digest changes.
  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
    ${CMAKE_COMMAND} -E touch "${manifest_path}"
  )
endif()
