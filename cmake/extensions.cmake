#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(get_board_without_ns_suffix board_in board_out)
  string(REGEX REPLACE "(_?ns)$" "" board_in_without_suffix ${board_in})
  if(NOT ${board_in} STREQUAL ${board_in_without_suffix})
    if (NOT CONFIG_ARM_NONSECURE_FIRMWARE)
      message(FATAL_ERROR "${board_in} is not a valid name for a board without "
      "'CONFIG_ARM_NONSECURE_FIRMWARE' set. This because the 'ns'/'_ns' ending "
      "indicates that the board is the non-secure variant in a TrustZone "
      "enabled system.")
    endif()
    set(${board_out} ${board_in_without_suffix} PARENT_SCOPE)
    message("Changed board to secure ${board_in_without_suffix} (NOT NS)")
  else()
    set(${board_out} ${board_in} PARENT_SCOPE)
  endif()
endfunction()

# Add an overlay file to a child image.
# This can be used by a parent image to set overlay of Kconfig configuration or devicetree
# in its child images. This function must be called before 'add_child_image(image)'
# to have effect.
#
# Parameters:
#   'image' - child image name
#   'overlay_file' - overlay to be added to child image
#   'overlay_type' - 'OVERLAY_CONFIG' or 'DTC_OVERLAY_FILE'
function(add_overlay image overlay_file overlay_type)
  set(old_overlays ${${image}_${overlay_type}})
  string(FIND "${old_overlays}" "${overlay_file}" found)
  if (${found} EQUAL -1)
    set(${image}_${overlay_type} "${old_overlays} ${overlay_file}" CACHE INTERNAL "")
  endif()
endfunction()

# Convenience macro to add configuration overlays to child image.
macro(add_overlay_config image overlay_file)
  add_overlay(${image} ${overlay_file} OVERLAY_CONFIG)
endmacro()

# Convenience macro to add device tree overlays to child image.
macro(add_overlay_dts image overlay_file)
  add_overlay(${image} ${overlay_file} DTC_OVERLAY_FILE)
endmacro()

# Add a partition manager configuration file to the build.
# Note that is only one image is included in the build,
# you must set CONFIG_PM_SINGLE_IMAGE=y for the partition manager
# configuration to take effect.
function(ncs_add_partition_manager_config config_file)
  get_filename_component(pm_path ${config_file} REALPATH)
  get_filename_component(pm_filename ${config_file} NAME)

  if (NOT EXISTS ${pm_path})
    message(FATAL_ERROR
      "Could not find specified partition manager configuration file "
      "${config_file} at ${pm_path}"
      )
  endif()

  set_property(GLOBAL APPEND PROPERTY
    PM_SUBSYS_PATHS
    ${pm_path}
    )
  set_property(GLOBAL APPEND PROPERTY
    PM_SUBSYS_OUTPUT_PATHS
    ${CMAKE_CURRENT_BINARY_DIR}/${pm_filename}
    )
endfunction()

# Usage:
#   ncs_file(<mode> <arg> ...)
#
# NCS file function extension.
# This function extends the zephyr_file(CONF_FILES <arg>) function to support
# switching BOARD for child images.
#
# This function currently support the following <modes>.
#
# BOARD <board>: Board name to use when searching for board specific Kconfig
#                fragments.
#
# CONF_FILES <path>: Find all configuration files in path and return them in a
#                    list. Configuration files will be:
#                    - DTS:       Overlay files (.overlay)
#                    - Kconfig:   Config fragments (.conf)
#                    The conf file search will return existing configuration
#                    files for BOARD or the current board if BOARD argument is
#                    not given.
#                    CONF_FILES takes the following additional arguments:
#                    BOARD <board>:             Find configuration files for specified board.
#                    BOARD_REVISION <revision>: Find configuration files for specified board
#                                               revision. Requires BOARD to be specified.
#
#                                               If no board is given the current BOARD and
#                                               BOARD_REVISION will be used.
#
#                    DTS <list>:   List to populate with DTS overlay files
#                    KCONF <list>: List to populate with Kconfig fragment files
#                    BUILD <type>: Build type to include for search.
#                                  For example:
#                                  BUILD debug, will look for <board>_debug.conf
#                                  and <board>_debug.overlay, instead of <board>.conf
#
function(ncs_file)
  set(file_options CONF_FILES)
  if((ARGC EQUAL 0) OR (NOT (ARGV0 IN_LIST file_options)))
    message(FATAL_ERROR "No <mode> given to `ncs_file(<mode> <args>...)` function,\n \
Please provide one of following: CONF_FILES")
  endif()

  set(single_args CONF_FILES)
  cmake_parse_arguments(FILE "" "${single_args}" "" ${ARGN})
  cmake_parse_arguments(ZEPHYR_FILE "" "KCONF;DTS;BUILD" "" ${ARGN})

  if(ZEPHYR_FILE_KCONF)
    if(ZEPHYR_FILE_BUILD AND EXISTS ${FILE_CONF_FILES}/prj_${ZEPHYR_FILE_BUILD}.conf)
      set(${ZEPHYR_FILE_KCONF} ${FILE_CONF_FILES}/prj_${ZEPHYR_FILE_BUILD}.conf)
    elseif(NOT ZEPHYR_FILE_BUILD AND EXISTS ${FILE_CONF_FILES}/prj.conf)
      set(${ZEPHYR_FILE_KCONF} ${FILE_CONF_FILES}/prj.conf)
    endif()
  endif()

  zephyr_file(CONF_FILES ${FILE_CONF_FILES}/boards ${FILE_UNPARSED_ARGUMENTS})
  if(ZEPHYR_FILE_DTS)
    zephyr_file(CONF_FILES ${FILE_CONF_FILES}
                ${ZEPHYR_FILE_UNPARSED_ARGUMENTS}
                DTS ${ZEPHYR_FILE_DTS})
  endif()

  if(ZEPHYR_FILE_KCONF)
    set(${ZEPHYR_FILE_KCONF} ${${ZEPHYR_FILE_KCONF}} PARENT_SCOPE)
  endif()

  if(ZEPHYR_FILE_DTS)
    set(${ZEPHYR_FILE_DTS} ${${ZEPHYR_FILE_DTS}} PARENT_SCOPE)
  endif()
endfunction()

# Usage:
#   target_compile_options_final(<target> <PRIVATE|PUBLIC|INTERFACE> [item(s)])
#
# NCS target_compile_options_final().
# This function extends the target_compile_options(<target> ...) function to
# support compile options to be applied at the end of compiler invocations.
#
# CMake will first place a targets own compile options, and thereafter inherited
# compile options from link libraries or interface libraries.
#
# This means that a compile option from an interface library may overrule the
# option specified on the library itself.
#
# Using target_compile_options_final() allows a user to specify options that
# must be placed AFTER inherited options.
#
# In order for target_compile_options_final() to work correctly then it is
# important that this function is called AFTER all target_link_libraries().
#
# Correct usage for target 'foo'
#   add_library(foo STATIC ...)
#   target_link_libraries(foo PRIVATE bar)
#   target_compile_options_final(foo PRIVATE -late-option)
#
# Wrong usage:
#   add_library(foo STATIC ...)
#   target_compile_options_final(foo PRIVATE -late-option)
#   # This results in options from 'bar' being applied after '-late-option'
#   target_link_libraries(foo PRIVATE bar)
#
function(target_compile_options_final)
  if(${ARGC} LESS 2)
    message(FATAL_ERROR "target_compile_options_final called with incorrect number of arguments")
  endif()

  set(VALID_KEYWORDS PRIVATE PUBLIC INTERFACE)
  if(NOT ${ARGV1} IN_LIST VALID_KEYWORDS)
    message(FATAL_ERROR "target_compile_options_final called with invalid arguments")
  endif()

  list(REMOVE_AT ARGV 0 1)

  if(${ARGV1} STREQUAL INTERFACE)
    target_compile_options(${ARGV0} INTERFACE ${ARGV})
    return()
  endif()

  if(${ARGV1} STREQUAL PUBLIC)
    target_compile_options(${ARGV0} INTERFACE ${ARGV})
  endif()

  get_property(libraries TARGET ${ARGV0} PROPERTY LINK_LIBRARIES)
  if(NOT compile_options_final IN_LIST libraries)
    set_property(
      TARGET ${ARGV0}
      APPEND PROPERTY LINK_LIBRARIES compile_options_final
    )
  endif()
  set_property(TARGET ${ARGV0} APPEND PROPERTY COMPILE_OPTIONS_FINAL ${ARGV})
endfunction()
