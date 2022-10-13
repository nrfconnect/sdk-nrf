# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

# Usage:
#   load_cache(IMAGE <image> BINARY_DIR <dir>)
#
# This function will load the CMakeCache.txt file from the binary directory
# given by the BINARY_DIR argument.
#
# All CMake cache variables are stored in a custom target which is identified by
# the name given as value to the IMAGE argument.
#
# IMAGE:      image name identifying the cache for later sysbuild_get() lookup calls.
# BINARY_DIR: binary directory (build dir) containing the CMakeCache.txt file to load.
function(load_cache)
  set(single_args IMAGE BINARY_DIR)
  cmake_parse_arguments(LOAD_CACHE "" "${single_args}" "" ${ARGN})

  if(NOT TARGET ${LOAD_CACHE_IMAGE}_cache)
    add_custom_target(${LOAD_CACHE_IMAGE}_cache)
  endif()
  file(STRINGS "${LOAD_CACHE_BINARY_DIR}/CMakeCache.txt" cache_strings)
  foreach(str ${cache_strings})
    # Using a regex for matching whole 'VAR_NAME:TYPE=VALUE' will strip semi-colons
    # thus resulting in lists to become strings.
    # Therefore we first fetch VAR_NAME and TYPE, and afterwards extract
    # remaining of string into a value that populates the property.
    # This method ensures that both quoted values and ;-separated list stays intact.
    string(REGEX MATCH "([^:]*):([^=]*)=" variable_identifier ${str})
    if(NOT "${variable_identifier}" STREQUAL "")
      string(LENGTH ${variable_identifier} variable_identifier_length)
      string(SUBSTRING "${str}" ${variable_identifier_length} -1 variable_value)
      set_property(TARGET ${LOAD_CACHE_IMAGE}_cache APPEND PROPERTY "CACHE:VARIABLES" "${CMAKE_MATCH_1}")
      set_property(TARGET ${LOAD_CACHE_IMAGE}_cache PROPERTY "${CMAKE_MATCH_1}:TYPE" "${CMAKE_MATCH_2}")
      set_property(TARGET ${LOAD_CACHE_IMAGE}_cache PROPERTY "${CMAKE_MATCH_1}" "${variable_value}")
    endif()
  endforeach()
endfunction()

# Usage:
#   sysbuild_get(<variable> IMAGE <image> [VAR <image-variable>])
#
# This function will return the variable found in the CMakeCache.txt file
# identified by image.
# If `VAR` is provided, the name given as parameter will be looked up, but if
# `VAR` is not given, the `<variable>` name provided will be used both for
# lookup and value return.
#
# The result will be returned in `<variable>`.
#
# Example use:
#   sysbuild_get(PROJECT_NAME IMAGE my_sample)
#     will lookup PROJECT_NAME from the CMakeCache identified by `my_sample` and
#     and return the value in the local variable `PROJECT_NAME`.
#
#   sysbuild_get(my_sample_PROJECT_NAME IMAGE my_sample VAR PROJECT_NAME)
#     will lookup PROJECT_NAME from the CMakeCache identified by `my_sample` and
#     and return the value in the local variable `my_sample_PROJECT_NAME`.
#
# <variable>: variable used for returning CMake cache value. Also used as lookup
#             variable if `VAR` is not provided.
# IMAGE:      image name identifying the cache to use for variable lookup.
# VAR:        name of the CMake cache variable name to lookup.
function(sysbuild_get variable)
  cmake_parse_arguments(GET_VAR "" "IMAGE;VAR" "" ${ARGN})

  if(NOT DEFINED GET_VAR_IMAGE)
    message(FATAL_ERROR "sysbuild_get(...) requires IMAGE.")
  endif()

  if(DEFINED ${variable})
    message(WARNING "Return variable ${variable} already defined with a value. "
                    "sysbuild_get(${variable} ...) may overwrite existing value. "
		    "Please use sysbuild_get(<variable> ... VAR <image-variable>) "
		    "where <variable> is undefined."
    )
  endif()

  if(NOT DEFINED GET_VAR_VAR)
    set(GET_VAR_VAR ${variable})
  endif()

  get_property(${GET_VAR_IMAGE}_${GET_VAR_VAR} TARGET ${GET_VAR_IMAGE}_cache PROPERTY ${GET_VAR_VAR})
  if(DEFINED ${GET_VAR_IMAGE}_${GET_VAR_VAR})
    set(${variable} ${${GET_VAR_IMAGE}_${GET_VAR_VAR}} PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   ExternalZephyrProject_Add(APPLICATION <name>
#                             SOURCE_DIR <dir>
#                             [BOARD <board>]
#                             [MAIN_APP]
#   )
#
# This function includes a Zephyr based build system into the multiimage
# build system
#
# APPLICATION: <name>: Name of the application, name will also be used for build
#                      folder of the application
# SOURCE_DIR <dir>:    Source directory of the application
# BOARD <board>:       Use <board> for application build instead user defined BOARD.
# MAIN_APP:            Flag indicating this application is the main application
#                      and where user defined settings should be passed on as-is
#                      except for multi image build flags.
#                      For example, -DCONF_FILES=<files> will be passed on to the
#                      MAIN_APP unmodified.
#
function(ExternalZephyrProject_Add)
  cmake_parse_arguments(ZBUILD "MAIN_APP" "APPLICATION;BOARD;SOURCE_DIR" "" ${ARGN})

  if(ZBUILD_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "ExternalZephyrProject_Add(${ARGV0} <val> ...) given unknown arguments:"
      " ${ZBUILD_UNPARSED_ARGUMENTS}"
    )
  endif()

  # CMake variables which must be known by all Zephyr CMake build systems
  # Those are settings which controls the build and must be known to CMake at
  # invocation time, and thus cannot be passed though the sysbuild cache file.
  set(
    shared_cmake_variables_list
    CMAKE_BUILD_TYPE
    CMAKE_VERBOSE_MAKEFILE
  )

  set(sysbuild_cache_file ${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION}_sysbuild_cache.txt)

  get_cmake_property(sysbuild_cache CACHE_VARIABLES)
  foreach(var_name ${sysbuild_cache})
    if(NOT "${var_name}" MATCHES "^CMAKE_.*")
      # We don't want to pass internal CMake variables.
      # Required CMake variable to be passed, like CMAKE_BUILD_TYPE must be
      # passed using `-D` on command invocation.
      get_property(var_type CACHE ${var_name} PROPERTY TYPE)
      set(cache_entry "${var_name}:${var_type}=${${var_name}}")
      string(REPLACE ";" "\;" cache_entry "${cache_entry}")
      list(APPEND sysbuild_cache_strings "${cache_entry}\n")
    endif()
  endforeach()
  list(APPEND sysbuild_cache_strings "SYSBUILD_NAME:STRING=${ZBUILD_APPLICATION}\n")

  if(ZBUILD_MAIN_APP)
    list(APPEND sysbuild_cache_strings "SYSBUILD_MAIN_APP:BOOL=True\n")
  endif()

  if(DEFINED ZBUILD_BOARD)
    # Only set image specific board if provided.
    # The sysbuild BOARD is exported through sysbuild cache, and will be used
    # unless <image>_BOARD is defined.
    list(APPEND sysbuild_cache_strings "${ZBUILD_APPLICATION}_BOARD:STRING=${ZBUILD_BOARD}\n")
  endif()

  file(WRITE ${sysbuild_cache_file}.tmp ${sysbuild_cache_strings})
  zephyr_file_copy(${sysbuild_cache_file}.tmp ${sysbuild_cache_file} ONLY_IF_DIFFERENT)

  set(shared_cmake_vars_argument)
  foreach(shared_var ${shared_cmake_variables_list})
    if(DEFINED CACHE{${ZBUILD_APPLICATION}_${shared_var}})
      get_property(var_type  CACHE ${ZBUILD_APPLICATION}_${shared_var} PROPERTY TYPE)
      list(APPEND shared_cmake_vars_argument
           "-D${shared_var}:${var_type}=$CACHE{${ZBUILD_APPLICATION}_${shared_var}}"
      )
    elseif(DEFINED CACHE{${shared_var}})
      get_property(var_type  CACHE ${shared_var} PROPERTY TYPE)
      list(APPEND shared_cmake_vars_argument
           "-D${shared_var}:${var_type}=$CACHE{${shared_var}}"
      )
    endif()
  endforeach()

  set(image_banner "* Running CMake for ${ZBUILD_APPLICATION} *")
  string(LENGTH "${image_banner}" image_banner_width)
  string(REPEAT "*" ${image_banner_width} image_banner_header)
  message(STATUS "\n   ${image_banner_header}\n"
                 "   ${image_banner}\n"
                 "   ${image_banner_header}\n"
  )

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -G${CMAKE_GENERATOR}
      -DSYSBUILD:BOOL=True
      -DSYSBUILD_CACHE:FILEPATH=${sysbuild_cache_file}
      ${shared_cmake_vars_argument}
      -B${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION}
      -S${ZBUILD_SOURCE_DIR}
    RESULT_VARIABLE   return_val
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  if(return_val)
    message(FATAL_ERROR
            "CMake configure failed for Zephyr project: ${ZBUILD_APPLICATION}\n"
            "Location: ${ZBUILD_SOURCE_DIR}"
    )
  endif()
  load_cache(IMAGE ${ZBUILD_APPLICATION} BINARY_DIR ${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION})

  foreach(kconfig_target
      menuconfig
      hardenconfig
      guiconfig
      ${EXTRA_KCONFIG_TARGETS}
      )

    if(NOT ZBUILD_MAIN_APP)
      set(image_prefix "${ZBUILD_APPLICATION}_")
    endif()

    add_custom_target(${image_prefix}${kconfig_target}
      ${CMAKE_MAKE_PROGRAM} ${kconfig_target}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION}
      USES_TERMINAL
      )
  endforeach()
  include(ExternalProject)
  ExternalProject_Add(
    ${ZBUILD_APPLICATION}
    SOURCE_DIR ${ZBUILD_SOURCE_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}/${ZBUILD_APPLICATION}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} --build .
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
  )
endfunction()
