#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# Helper macro for verifying that at least one of the required arguments has
# been provided by the caller.
#
# As FATAL_ERROR will be raised if not one of the required arguments has been
# passed by the caller.
#
# Usage:
#   check_arguments_required(<function_name> <prefix> <arg1> [<arg2> ...])
#
macro(check_arguments_required function prefix)
  set(required_found FALSE)
  foreach(required ${ARGN})
    if(DEFINED ${prefix}_${required})
      set(required_found TRUE)
    endif()
  endforeach()

  if(NOT required_found)
    message(FATAL_ERROR "${function}(...) missing a required argument: ${ARGN}")
  endif()
endmacro()

#
# Helper macro for verifying that all the required arguments has # been
# provided by the caller.
#
# As FATAL_ERROR will be raised if one of the required arguments is missing.
#
# Usage:
#   check_arguments_required_all(<function_name> <prefix> <arg1> [<arg2> ...])
#
macro(check_arguments_required_all function prefix)
  foreach(required ${ARGN})
    if(NOT DEFINED ${prefix}_${required})
      message(FATAL_ERROR "${function}(...) missing a required argument: ${required}")
    endif()
  endforeach()
endmacro()

#
# Helper macro for verifying that none of the mutual exclusive arguments are
# provided together with the first argument.
#
# As FATAL_ERROR will be raised if first argument is given together with one
# of the following mutual exclusive arguments.
#
# Usage:
#   check_arguments_exclusive(<function_name> <prefix> <arg1> <exlude-arg1> [<exclude-arg2> ...])
#
macro(check_arguments_exclusive function prefix argument)
  foreach(prohibited ${ARGN})
    if(DEFINED ${prefix}_${argument} AND ${prefix}_${prohibited})
      message(FATAL_ERROR "set_shared(${argument} ...) cannot be used with "
        "argument: ${prohibited}"
      )
    endif()
  endforeach()
endmacro()

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
    set(${image}_${overlay_type} "${old_overlays};${overlay_file}" CACHE STRING
      "Extra config fragments for ${image} child image" FORCE
    )
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
# It also supports lookup of static partition manager files for based on
# the board name, revision, and the current build type.
# The order at which files are considers is from the most specific to the least specific:
# - first, file name with board name, board revision and build type identifiers,
# - second, file name with board name and build type identifiers,
# - third, file with only build type identifier,
# - finally, the file with no identifiers is looked up.
# During each pass, if domain is defined, the file with additional domain identifier has precedence.
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
#                    PM <list>:    List to populate with board / build / domain specific
#                                  static partition manager files
#                    BUILD <type>: Build type to include for search.
#                                  For example:
#                                  BUILD debug, will look for <board>_debug.conf
#                                  and <board>_debug.overlay, instead of <board>.conf
#                    DOMAIN <domain>: Domain to use. This argument is only effective
#                                     for partition manager configuration files.
#
function(ncs_file)
  set(file_options CONF_FILES)
  if((ARGC EQUAL 0) OR (NOT (ARGV0 IN_LIST file_options)))
    message(FATAL_ERROR "No <mode> given to `ncs_file(<mode> <args>...)` function,\n \
Please provide one of following: CONF_FILES")
  endif()

  set(single_args CONF_FILES PM DOMAIN)
  set(zephyr_conf_single_args BOARD BOARD_REVISION BUILD DTS KCONF)

  cmake_parse_arguments(PREPROCESS_ARGS "" "${single_args};${zephyr_conf_single_args}" "" ${ARGN})
  # Remove any argument that is missing value to ensure proper behavior in situations like:
  # ncs_file(CONF_FILES <path> PM <list> DOMAIN BUILD <type>)
  # where value of DOMAIN could wrongly become BUILD which is another keyword.
  if(DEFINED PREPROCESS_ARGS_KEYWORDS_MISSING_VALUES)
    list(REMOVE_ITEM ARGN ${PREPROCESS_ARGS_KEYWORDS_MISSING_VALUES})
  endif()

  cmake_parse_arguments(NCS_FILE "" "${single_args}" "" ${ARGN})
  cmake_parse_arguments(ZEPHYR_FILE "" "${zephyr_conf_single_args}" "" ${ARGN})

  if(ZEPHYR_FILE_KCONF)
    if(ZEPHYR_FILE_BUILD AND EXISTS ${NCS_FILE_CONF_FILES}/prj_${ZEPHYR_FILE_BUILD}.conf)
      set(${ZEPHYR_FILE_KCONF} ${NCS_FILE_CONF_FILES}/prj_${ZEPHYR_FILE_BUILD}.conf)
    elseif(NOT ZEPHYR_FILE_BUILD AND EXISTS ${NCS_FILE_CONF_FILES}/prj.conf)
      set(${ZEPHYR_FILE_KCONF} ${NCS_FILE_CONF_FILES}/prj.conf)
    endif()
  endif()

  zephyr_file(CONF_FILES ${NCS_FILE_CONF_FILES}/boards ${NCS_FILE_UNPARSED_ARGUMENTS})

  if(ZEPHYR_FILE_KCONF)
    set(${ZEPHYR_FILE_KCONF} ${${ZEPHYR_FILE_KCONF}} PARENT_SCOPE)
  endif()

  if(ZEPHYR_FILE_DTS)
    set(${ZEPHYR_FILE_DTS} ${${ZEPHYR_FILE_DTS}} PARENT_SCOPE)
  endif()

  if(NOT DEFINED ZEPHYR_FILE_BOARD)
    # Defaulting to system wide settings when BOARD is not given as argument
    set(ZEPHYR_FILE_BOARD ${BOARD})
    if(DEFINED BOARD_REVISION)
      set(ZEPHYR_FILE_BOARD_REVISION ${BOARD_REVISION})
    endif()
  endif()

  if(NCS_FILE_PM)
    set(PM_FILE_PREFIX pm_static)

    # Prepare search for pm_static_board@ver_build.yml
    zephyr_build_string(filename
                        BOARD ${ZEPHYR_FILE_BOARD}
                        BOARD_REVISION ${ZEPHYR_FILE_BOARD_REVISION}
                        BUILD ${ZEPHYR_FILE_BUILD}
    )
    set(filename_list ${PM_FILE_PREFIX}_${filename})

    # Prepare search for pm_static_board_build.yml
    zephyr_build_string(filename
                        BOARD ${ZEPHYR_FILE_BOARD}
                        BUILD ${ZEPHYR_FILE_BUILD}
    )
    list(APPEND filename_list ${PM_FILE_PREFIX}_${filename})

    # Prepare search for pm_static_build.yml
    # Note that BOARD argument is used to position suffix accordingly
    zephyr_build_string(filename
                        BOARD ${ZEPHYR_FILE_BUILD}
    )
    list(APPEND filename_list ${PM_FILE_PREFIX}_${filename})

    # Prepare search for pm_static.yml
    list(APPEND filename_list ${PM_FILE_PREFIX})
    list(REMOVE_DUPLICATES filename_list)

    foreach(filename ${filename_list})
      if(DEFINED NCS_FILE_DOMAIN)
        if(EXISTS ${NCS_FILE_CONF_FILES}/${filename}_${NCS_FILE_DOMAIN}.yml)
          set(${NCS_FILE_PM} ${NCS_FILE_CONF_FILES}/${filename}_${NCS_FILE_DOMAIN}.yml PARENT_SCOPE)
          break()
        endif()
      endif()

      if(EXISTS ${NCS_FILE_CONF_FILES}/${filename}.yml)
        set(${NCS_FILE_PM} ${NCS_FILE_CONF_FILES}/${filename}.yml PARENT_SCOPE)
        break()
      endif()
    endforeach()
  endif()

endfunction()

#
# Usage
#   set_shared(IMAGE <img> [APPEND] PROPERTY <property> <value>)
#
# Shares a property from child to parent.
# The property is shared through an intermediate shared_vars.cmake file which
# will be parsed by the parent image at CMake configure time.
#
# Example usage 'set_shared(IMAGE child PROPERTY visible_in_parent "I AM YOUR CHILD")'
#
# Usage
#   set_shared(FILE <file>)
#
# Shares all properties in file to parent.
# This function can be used to re-share properties from a child to its
# grand parent.
#
function(set_shared)
  set(flags       "APPEND")
  set(single_args "FILE;IMAGE")
  set(multi_args  "PROPERTY")
  cmake_parse_arguments(SHARE "${flags}" "${single_args}" "${multi_args}" ${ARGN})

  if(SYSBUILD)
    # Sysbuild can read the cache directly, no reason for an extra share file.
    list(POP_FRONT SHARE_PROPERTY listname)
    if(SHARE_APPEND)
      list(APPEND ${listname} ${SHARE_PROPERTY})
      list(REMOVE_DUPLICATES ${listname})
      set(SHARE_PROPERTY ${${listname}})
    endif()
    set(${listname} "${SHARE_PROPERTY}" CACHE INTERNAL "shared var")
    return()
  endif()

  check_arguments_required("set_shared" SHARE IMAGE FILE)

  check_arguments_exclusive("set_shared" SHARE FILE IMAGE PROPERTY APPEND)
  check_arguments_exclusive("set_shared" SHARE IMAGE FILE)


  set(prop_target ${IMAGE_NAME}_shared_property_target)
  if(NOT TARGET ${prop_target})
    add_custom_target(${prop_target})
  endif()

  if(DEFINED SHARE_IMAGE)
    # When using IMAGE, then PROPERTY is also required.
    check_arguments_required("set_shared" SHARE PROPERTY)

    set(share_prop_target ${SHARE_IMAGE}_shared_property_target)

    if(SHARE_APPEND)
      set(SHARE_APPEND APPEND)
    else()
      set(SHARE_APPEND)
    endif()

    get_property(string_targets TARGET ${prop_target} PROPERTY image_targets)
    if(NOT "add_custom_target(${share_prop_target})" IN_LIST string_targets)
      set_property(
        TARGET ${prop_target} APPEND PROPERTY
        image_targets "add_custom_target(${share_prop_target})"
      )
    endif()

    set_property(TARGET ${prop_target} APPEND_STRING PROPERTY shared_vars
      "set_property(TARGET ${share_prop_target} ${SHARE_APPEND} PROPERTY ${SHARE_PROPERTY})\n"
    )
  endif()

  if(DEFINED SHARE_FILE)
    set_property(TARGET ${prop_target} APPEND_STRING PROPERTY shared_vars
      "include(${SHARE_FILE})\n"
    )
  endif()
endfunction()

# generate_shared(IMAGE <img> FILE <file>)
function(generate_shared)
  set(single_args "IMAGE;FILE")
  cmake_parse_arguments(SHARE "" "${single_args}" "" ${ARGN})

  check_arguments_required_all("generate_shared" SHARE IMAGE FILE)

  set(prop_target ${IMAGE_NAME}_shared_property_target)
  file(GENERATE OUTPUT ${SHARE_FILE}
      CONTENT
        "$<JOIN:$<TARGET_PROPERTY:${prop_target},image_targets>,\n>
$<TARGET_PROPERTY:${prop_target},shared_vars>"
    )
endfunction()

#
# Usage
#   get_shared(<var> IMAGE <img> PROPERTY <property>)
#
# Get a property value defined by the child image or domain <img> if it exists.
# The property value will be returned in the variable referenced by <var>.
#
# Example usage 'get_shared(prop_value IMAGE child PROPERTY property_in_child)'
#
function(get_shared var)
  set(single_args "IMAGE")
  set(multi_args  "PROPERTY")
  cmake_parse_arguments(SHARE "" "${single_args}" "${multi_args}" ${ARGN})

  check_arguments_required_all("get_shared" SHARE IMAGE PROPERTY)

  if(TARGET ${SHARE_IMAGE}_shared_property_target)
    get_property(
      ${var}
      TARGET   ${SHARE_IMAGE}_shared_property_target
      PROPERTY ${SHARE_PROPERTY}
    )
    set(${var} ${${var}} PARENT_SCOPE)
  endif()
endfunction()
