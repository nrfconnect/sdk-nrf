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
#     BINARY_PATH <path>
#     [NAME <name>]
#     [VERSION <version>]
#     [PACKAGE_TYPE <zip|multi|all>]
#     [DEPENDS <target1> [<target2> ...]]
#   )
#
# Parameters:
#   BINARY_PATH  - Path to the binary file to include in the package.
#   NAME         - Optional name for the binary (used in packages and as partition name).
#                  Defaults to basename of BINARY_PATH.
#   VERSION      - Optional image version string (for signing).
#   PACKAGE_TYPE - Optional package type selection: "zip", "multi", or "all" (default: "all")
#                  Controls which package types include this binary.
#   DEPENDS      - Optional list of CMake targets that must be built before
#                  this extra binary is available
#
# Notes:
#   Image IDs are automatically assigned when dfu_extra_get_binaries() is called.
#
function(dfu_extra_add_binary)
  cmake_parse_arguments(EXTRA "" "BINARY_PATH;NAME;VERSION;PACKAGE_TYPE" "DEPENDS" ${ARGN})

  # Validate required parameters
  if(NOT DEFINED EXTRA_BINARY_PATH)
    message(FATAL_ERROR "BINARY_PATH is required")
  endif()

  # Set defaults for optional parameters
  if(NOT DEFINED EXTRA_NAME)
    get_filename_component(EXTRA_NAME ${EXTRA_BINARY_PATH} NAME)
  endif()

  if(NOT DEFINED EXTRA_PACKAGE_TYPE)
    set(EXTRA_PACKAGE_TYPE "all")
  endif()

  # Handle version
  if(NOT DEFINED EXTRA_VERSION)
    set(EXTRA_VERSION "0.0.0+0")
  endif()

  # Validate PACKAGE_TYPE parameter
  if(NOT EXTRA_PACKAGE_TYPE MATCHES "^(zip|multi|all)$")
    message(WARNING "PACKAGE_TYPE must be 'zip', 'multi', or 'all', got: ${EXTRA_PACKAGE_TYPE}")
    return()
  endif()

  # Prepare target list for dependencies
  if(EXTRA_DEPENDS)
    set(target_list "${EXTRA_DEPENDS}")
  else()
    set(target_list "${EXTRA_BINARY_PATH}")
  endif()

  get_property(all_paths GLOBAL PROPERTY DFU_EXTRA_PATHS)
  get_property(all_names GLOBAL PROPERTY DFU_EXTRA_NAMES)
  get_property(all_versions GLOBAL PROPERTY DFU_EXTRA_VERSIONS)
  get_property(all_package_types GLOBAL PROPERTY DFU_EXTRA_PACKAGE_TYPES)
  get_property(all_targets GLOBAL PROPERTY DFU_EXTRA_TARGETS)

  list(APPEND all_paths "${EXTRA_BINARY_PATH}")
  list(APPEND all_names "${EXTRA_NAME}")
  list(APPEND all_versions "${EXTRA_VERSION}")
  list(APPEND all_package_types "${EXTRA_PACKAGE_TYPE}")
  list(APPEND all_targets "${target_list}")

  set_property(GLOBAL PROPERTY DFU_EXTRA_PATHS "${all_paths}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_NAMES "${all_names}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_PACKAGE_TYPES "${all_package_types}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_VERSIONS "${all_versions}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_TARGETS "${all_targets}")
endfunction()

#
# Get all extra binary information for DFU packaging
#
# This function retrieves all extra binaries registered via dfu_extra_add_binary().
# It returns comprehensive information that can be used by packaging systems.
# Image IDs are automatically assigned here based on the order binaries were added.
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
    message(WARNING "IDS, PATHS, and TARGETS are required")
    return()
  endif()

  get_property(all_paths GLOBAL PROPERTY DFU_EXTRA_PATHS)
  get_property(all_names GLOBAL PROPERTY DFU_EXTRA_NAMES)
  get_property(all_package_types GLOBAL PROPERTY DFU_EXTRA_PACKAGE_TYPES)
  get_property(all_targets GLOBAL PROPERTY DFU_EXTRA_TARGETS)

  foreach(var IN ITEMS all_paths all_names all_package_types all_targets)
    if(NOT ${var})
      set(${var} "")
    endif()
  endforeach()

  set(all_image_ids "")
  list(LENGTH all_paths num_binaries)
  if(num_binaries GREATER 0)
    math(EXPR last_index "${num_binaries} - 1")
    foreach(index RANGE ${last_index})
      list(GET all_paths ${index} current_path)

      # Calculate slot number (starts from 1)
      math(EXPR slot_number "${index} + 1")

      # Check if the specific slot is configured
      if(NOT DEFINED NCS_MCUBOOT_EXTRA_${slot_number}_IMAGE_NUMBER OR NCS_MCUBOOT_EXTRA_${slot_number}_IMAGE_NUMBER EQUAL -1)
        message(WARNING "Extra image slot ${slot_number} not configured. Set SB_CONFIG_MCUBOOT_EXTRA_IMAGES to at least ${num_binaries} in sysbuild.")
        return()
      endif()

      set(current_id ${NCS_MCUBOOT_EXTRA_${slot_number}_IMAGE_NUMBER})
      list(APPEND all_image_ids ${current_id})
    endforeach()
  endif()

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
