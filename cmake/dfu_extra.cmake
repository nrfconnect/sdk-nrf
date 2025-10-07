#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# DFU Extra Image extension
#
# This file provides functions for adding extra images which are not natively
# integrated into nRF Connect SDK to the DFU packages.
# It supports both multi-image binaries and ZIP packages, allowing applications
# to extend the built-in DFU functionality with additional extra firmware images.
#

#
# Add an extra binary to DFU packages (multi-image binary and/or ZIP)
#
# Usage:
#   dfu_extra_add_binary(
#     IMAGE_ID <id>
#     BINARY_PATH <path>
#     [IMAGE_NAME <name>]
#     [PACKAGE_TYPE <zip|multi|all>]
#     [DEPENDS <target1> [<target2> ...]]
#   )
#
# Parameters:
#   IMAGE_ID     - Image identifier. Must be unique and should not conflict with built-in IDs.
#   BINARY_PATH  - Path to the binary file to include in the package.
#   IMAGE_NAME   - Optional name for the binary in packages (defaults to basename of BINARY_PATH)
#   PACKAGE_TYPE - Optional package type selection: "zip", "multi", or "all" (default: "all")
#                  Controls which package types include this binary.
#   DEPENDS      - Optional list of CMake targets that must be built before
#                  this extra binary is available
#
function(dfu_extra_add_binary)
  cmake_parse_arguments(EXTRA "" "IMAGE_ID;BINARY_PATH;IMAGE_NAME;PACKAGE_TYPE" "DEPENDS" ${ARGN})

  # Validate required parameters
  if(NOT DEFINED EXTRA_IMAGE_ID OR NOT DEFINED EXTRA_BINARY_PATH)
    message(FATAL_ERROR "IMAGE_ID and BINARY_PATH are required")
  endif()

  # Set defaults for optional parameters
  if(NOT DEFINED EXTRA_IMAGE_NAME)
    get_filename_component(EXTRA_IMAGE_NAME ${EXTRA_BINARY_PATH} NAME)
  endif()

  if(NOT DEFINED EXTRA_PACKAGE_TYPE)
    set(EXTRA_PACKAGE_TYPE "all")
  endif()

  # Validate PACKAGE_TYPE parameter
  if(NOT EXTRA_PACKAGE_TYPE MATCHES "^(zip|multi|all)$")
    message(FATAL_ERROR "PACKAGE_TYPE must be 'zip', 'multi', or 'all', got: ${EXTRA_PACKAGE_TYPE}")
  endif()

  # Prepare target list for dependencies
  if(EXTRA_DEPENDS)
    set(target_list "${EXTRA_DEPENDS}")
  else()
    set(target_list "")
  endif()

  get_property(all_paths GLOBAL PROPERTY DFU_EXTRA_PATHS)
  get_property(all_names GLOBAL PROPERTY DFU_EXTRA_NAMES)
  get_property(all_targets GLOBAL PROPERTY DFU_EXTRA_TARGETS)
  get_property(all_image_ids GLOBAL PROPERTY DFU_EXTRA_IMAGE_IDS)
  get_property(all_package_types GLOBAL PROPERTY DFU_EXTRA_PACKAGE_TYPES)

  list(APPEND all_paths "${EXTRA_BINARY_PATH}")
  list(APPEND all_names "${EXTRA_IMAGE_NAME}")
  list(APPEND all_targets "${target_list}")
  list(APPEND all_image_ids "${EXTRA_IMAGE_ID}")
  list(APPEND all_package_types "${EXTRA_PACKAGE_TYPE}")

  set_property(GLOBAL PROPERTY DFU_EXTRA_PATHS "${all_paths}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_NAMES "${all_names}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_TARGETS "${all_targets}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_IMAGE_IDS "${all_image_ids}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_PACKAGE_TYPES "${all_package_types}")
endfunction()

#
# Get all extra binary information for DFU packaging
#
# This function retrieves all extra binaries registered via dfu_extra_add_binary().
# It returns comprehensive information that can be used by packaging systems.
#
# Parameters:
#   IDS             - Output variable name for list of image IDs
#   PATHS           - Output variable name for list of binary file paths
#   TARGETS         - Output variable name for list of CMake target dependencies
#   NAMES           - Output variable name for list of binary names
#   FILTER_PACKAGES - Optional filter: "zip", "multi", or "all" (default: no filter, return all)
#
function(dfu_extra_get_binaries)
  cmake_parse_arguments(GET "" "IDS;PATHS;TARGETS;NAMES;FILTER_PACKAGES" "" ${ARGN})

  # Validate required parameters
  if(NOT DEFINED GET_IDS OR NOT DEFINED GET_PATHS OR NOT DEFINED GET_TARGETS)
    message(FATAL_ERROR "IDS, PATHS, and TARGETS are required")
  endif()

  get_property(all_paths GLOBAL PROPERTY DFU_EXTRA_PATHS)
  get_property(all_names GLOBAL PROPERTY DFU_EXTRA_NAMES)
  get_property(all_targets GLOBAL PROPERTY DFU_EXTRA_TARGETS)
  get_property(all_image_ids GLOBAL PROPERTY DFU_EXTRA_IMAGE_IDS)
  get_property(all_package_types GLOBAL PROPERTY DFU_EXTRA_PACKAGE_TYPES)

  foreach(var IN ITEMS all_paths all_names all_targets all_image_ids all_package_types)
    if(NOT ${var})
      set(${var} "")
    endif()
  endforeach()

  # Apply filter if specified
  if(DEFINED GET_FILTER_PACKAGES)
    foreach(var IN ITEMS filtered_paths filtered_names filtered_targets filtered_image_ids)
      set(${var} "")
    endforeach()

    # Filter binaries based on package type
    list(LENGTH all_paths count)
    if(count GREATER 0)
      math(EXPR last_index "${count} - 1")
      foreach(index RANGE ${last_index})
        list(GET all_package_types ${index} package_type)

        # Include if package_type matches filter or package_type is "all"
        if(package_type STREQUAL "${GET_FILTER_PACKAGES}" OR package_type STREQUAL "all")
          # Get all values for this index
          list(GET all_paths ${index} path)
          list(GET all_names ${index} name)
          list(GET all_targets ${index} target)
          list(GET all_image_ids ${index} image_id)

          # Append to filtered lists
          list(APPEND filtered_paths "${path}")
          list(APPEND filtered_names "${name}")
          list(APPEND filtered_targets "${target}")
          list(APPEND filtered_image_ids "${image_id}")
        endif()
      endforeach()
    endif()

    # Return filtered data
    set(${GET_PATHS} "${filtered_paths}" PARENT_SCOPE)
    if(DEFINED GET_NAMES)
      set(${GET_NAMES} "${filtered_names}" PARENT_SCOPE)
    endif()
    set(${GET_TARGETS} "${filtered_targets}" PARENT_SCOPE)
    set(${GET_IDS} "${filtered_image_ids}" PARENT_SCOPE)
  else()
    set(${GET_PATHS} "${all_paths}" PARENT_SCOPE)
    if(DEFINED GET_NAMES)
      set(${GET_NAMES} "${all_names}" PARENT_SCOPE)
    endif()
    set(${GET_TARGETS} "${all_targets}" PARENT_SCOPE)
    set(${GET_IDS} "${all_image_ids}" PARENT_SCOPE)
  endif()
endfunction()
