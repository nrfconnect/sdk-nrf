#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file provides utilities for tracking OpenThread Kconfig options and
# warning users when they modify options that have no effect with precompiled libraries.

# Include nrfxlib extensions to get access to openthread_calculate_lib_path
include(${ZEPHYR_NRFXLIB_MODULE_DIR}/openthread/cmake/extensions.cmake)

# Initialize the lists to track OpenThread Kconfig options and their OT mappings
# Format: "KCONFIG_OPTION:OT_OPTION" pairs
set(OPENTHREAD_KCONFIG_OT_BOOL_MAPPINGS "" CACHE INTERNAL "List of boolean Kconfig to OT option mappings")
set(OPENTHREAD_KCONFIG_OT_STRING_MAPPINGS "" CACHE INTERNAL "List of string Kconfig to OT option mappings")

# Unified macro that registers a Kconfig option and sets the corresponding OT option when building from sources
#
# Arguments:
#   kconfig_option - The Kconfig option to check (e.g., CONFIG_OPENTHREAD_ANYCAST_LOCATOR)
#   ot_config      - The OpenThread config to set (e.g., OT_ANYCAST_LOCATOR)
#   description    - Description for the cache variable
#   type           - Type of the option: BOOL, STRING, or INT
#
macro(kconfig_to_ot_option kconfig_option ot_config description type)
  if("${type}" STREQUAL "BOOL")
    # Register boolean option in the bool tracking list
    set(OPENTHREAD_KCONFIG_OT_BOOL_MAPPINGS "${OPENTHREAD_KCONFIG_OT_BOOL_MAPPINGS};${kconfig_option}:${ot_config}" CACHE INTERNAL "")

    # Only set the OT option when building OpenThread from sources
    if(CONFIG_OPENTHREAD_SOURCES)
      if(${kconfig_option})
        set(${ot_config} ON CACHE BOOL "${description}" FORCE)
      else()
        set(${ot_config} OFF CACHE BOOL "${description}" FORCE)
      endif()
    endif()
  elseif("${type}" STREQUAL "STRING" OR "${type}" STREQUAL "INT")
    # Register string/int option in the string tracking list (CMake treats both as strings)
    set(OPENTHREAD_KCONFIG_OT_STRING_MAPPINGS "${OPENTHREAD_KCONFIG_OT_STRING_MAPPINGS};${kconfig_option}:${ot_config}" CACHE INTERNAL "")

    # Only set the OT option when building OpenThread from sources
    if(CONFIG_OPENTHREAD_SOURCES)
      if(DEFINED ${kconfig_option})
        set(${ot_config} "${${kconfig_option}}" CACHE STRING "${description}" FORCE)
      else()
        # Explicitly set to empty if the Kconfig option is not defined
        set(${ot_config} "" CACHE STRING "${description}" FORCE)
      endif()
    endif()
  else()
    message(FATAL_ERROR "kconfig_to_ot_option: Unknown type '${type}'. Must be BOOL, STRING, or INT.")
  endif()
endmacro()

# Function to check and warn about modified OpenThread Kconfig options when using precompiled libraries
#
# This function should be called after all kconfig_to_ot_option() calls.
# It reads the library configuration file for the selected platform, compares current Kconfig values
# with the library OT_* values, and displays a warning if any options have been changed.
#
function(openthread_check_kconfig_for_precompiled_libs)
  # Calculate the library path based on platform and configuration
  openthread_calculate_lib_path("v${CONFIG_OPENTHREAD_THREAD_VERSION}" OT_LIB_PATH)

  # Check if configuration file exists
  set(conf_file "${OT_LIB_PATH}/openthread_lib_configuration.txt")
  if(NOT EXISTS "${conf_file}")
    message(WARNING "OpenThread library configuration file not found: ${conf_file}")
    return()
  endif()

  # Read the library configuration file
  file(READ "${conf_file}" LIB_CONFIG_CONTENT)

  # Parse the configuration file into lines
  string(REPLACE "\n" ";" conf_lines "${LIB_CONFIG_CONTENT}")

  # Collect all options that differ from the library configuration
  set(changed_options "")

  # Check boolean options
  foreach(mapping IN LISTS OPENTHREAD_KCONFIG_OT_BOOL_MAPPINGS)
    # Skip empty entries
    if("${mapping}" STREQUAL "")
      continue()
    endif()

    # Parse the mapping "KCONFIG_OPTION:OT_OPTION"
    string(REPLACE ":" ";" mapping_parts "${mapping}")
    list(LENGTH mapping_parts parts_count)
    if(NOT parts_count EQUAL 2)
      continue()
    endif()

    list(GET mapping_parts 0 kconfig_option)
    list(GET mapping_parts 1 ot_option)

    # Get current kconfig value: y -> ON, n -> OFF
    if(${kconfig_option})
      set(current_value "ON")
    else()
      set(current_value "OFF")
    endif()

    # Search for the OT_* option in the library configuration
    set(lib_value_found FALSE)
    set(lib_value "")
    foreach(line IN LISTS conf_lines)
      # Match lines like OT_COAP=ON, OT_ASSERT=OFF, or OT_LOG_LEVEL= (empty)
      if("${line}" MATCHES "^${ot_option}=(.*)$")
        set(lib_value_found TRUE)
        set(lib_value "${CMAKE_MATCH_1}")
        break()
      endif()
    endforeach()

    # Process library value: empty values are treated as OFF
    if(lib_value_found)
      if("${lib_value}" STREQUAL "")
        set(lib_value "OFF")
      endif()

      # Compare current value with library value
      if(NOT "${current_value}" STREQUAL "${lib_value}")
        list(APPEND changed_options "${kconfig_option} -> ${ot_option} (current: ${current_value}, library: ${lib_value})")
      endif()
    else()
      if("${current_value}" STREQUAL "ON")
        list(APPEND changed_options "${kconfig_option} -> ${ot_option} (current: ${current_value}, library: OFF)")
      endif()
    endif()
  endforeach()

  # Check string options
  foreach(mapping IN LISTS OPENTHREAD_KCONFIG_OT_STRING_MAPPINGS)
    # Skip empty entries
    if("${mapping}" STREQUAL "")
      continue()
    endif()

    # Parse the mapping "KCONFIG_OPTION:OT_OPTION"
    string(REPLACE ":" ";" mapping_parts "${mapping}")
    list(LENGTH mapping_parts parts_count)
    if(NOT parts_count EQUAL 2)
      continue()
    endif()

    list(GET mapping_parts 0 kconfig_option)
    list(GET mapping_parts 1 ot_option)

    # Get current kconfig string value
    set(current_value "${${kconfig_option}}")

    # Search for the OT_* option in the library configuration
    set(lib_value_found FALSE)
    set(lib_value "")
    foreach(line IN LISTS conf_lines)
      # Match lines like OT_POWER_SUPPLY=EXTERNAL or OT_EXTERNAL_MBEDTLS=mbedtls_external
      if("${line}" MATCHES "^${ot_option}=(.*)$")
        set(lib_value_found TRUE)
        set(lib_value "${CMAKE_MATCH_1}")
        break()
      endif()
    endforeach()

    # Compare current value with library value
    if(lib_value_found)
      if(NOT "${current_value}" STREQUAL "${lib_value}")
        list(APPEND changed_options "${kconfig_option} -> ${ot_option} (current: ${current_value}, library: ${lib_value})")
      endif()
    else()
      # OT option not found in library config
      # Only report if current value is not empty
      if(NOT "${current_value}" STREQUAL "")
        list(APPEND changed_options "${kconfig_option} -> ${ot_option} (current: ${current_value}, library: not set)")
      endif()
    endif()
  endforeach()

  list(LENGTH changed_options num_changed)

  if(num_changed GREATER 0)
    # Get relative path to the library
    cmake_path(RELATIVE_PATH OT_LIB_PATH BASE_DIRECTORY ${ZEPHYR_NRFXLIB_MODULE_DIR} OUTPUT_VARIABLE LIB_PATH_REL)

    # Format the list of options for display
    string(REPLACE ";" "\n #   - " options_formatted "${changed_options}")
    message(WARNING
      "\n"
      " ##################################################################################################\n"
      " #                                                                                                #\n"
      " # The following OpenThread Kconfig options differ from the precompiled library:                  #\n"
      " #   - ${options_formatted}\n"
      " #                                                                                                #\n"
      " # These options have NO EFFECT for OpenThread stack when using precompiled OpenThread libraries. #\n"
      " # However, they are still valid for platform or application files.                               #\n"
      " #                                                                                                #\n"
      " # The library was built with the configuration from:                                             #\n"
      " #   nrfxlib/${LIB_PATH_REL}/openthread_lib_configuration.txt\n"
      " #                                                                                                #\n"
      " # To apply these configuration changes, you need to build OpenThread from                        #\n"
      " # sources by setting CONFIG_OPENTHREAD_SOURCES=y in your configuration.                          #\n"
      " #                                                                                                #\n"
      " ##################################################################################################\n"
    )
  endif()
endfunction()
