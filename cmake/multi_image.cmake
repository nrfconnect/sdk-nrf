#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(IMAGE_NAME)
  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "set(${IMAGE_NAME}KERNEL_HEX_NAME ${KERNEL_HEX_NAME})\n"
    )

  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "list(APPEND ${IMAGE_NAME}BUILD_BYPRODUCTS ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME})\n"
    )

  file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/shared_vars.cmake
    CONTENT $<TARGET_PROPERTY:zephyr_property_target,shared_vars>
    )
endif(IMAGE_NAME)

function(image_board_selection board_in board_out)
  # It is assumed that only the root app will be built as non-secure.
  # This is not a valid assumption as there might be multiple non-secure
  # images defined.
  # TODO: Allow multiple non-secure images by using Kconfig to set the
  # secure/non-secure property rather than using a separate board definition.
  set(nonsecure_boards_with_ns_suffix
    nrf9160_pca10090ns
    nrf9160_pca20035ns
    )
  if(${board_in} IN_LIST nonsecure_boards_with_ns_suffix)
    string(LENGTH ${board_in} len)
    math(EXPR len_without_suffix "${len} - 2")
    string(SUBSTRING ${board_in} 0 ${len_without_suffix} board_in_without_suffix)

    set(${board_out} ${board_in_without_suffix} PARENT_SCOPE)
    message("Changed board to secure ${board_in_without_suffix} (NOT NS)")

  elseif(${board_in} STREQUAL actinius_icarus_ns)
    set(${board_out} actinius_icarus PARENT_SCOPE)
    message("Changed board to secure actinius_icarus (NOT NS)")

  else()
    set(${board_out} ${board_in} PARENT_SCOPE)
  endif()
endfunction()

function(add_child_image name sourcedir)
  string(TOUPPER ${name} UPNAME)

  if (CONFIG_${UPNAME}_BUILD_STRATEGY_USE_HEX_FILE)
    assert_exists(CONFIG_${UPNAME}_HEX_FILE)
    message("Using ${CONFIG_${UPNAME}_HEX_FILE} instead of building ${name}")

    # Set property so that the hex file is merged in by partition manager.
    set_property(GLOBAL PROPERTY ${name}_PM_HEX_FILE ${CONFIG_${UPNAME}_HEX_FILE})
  elseif (CONFIG_${UPNAME}_BUILD_STRATEGY_SKIP_BUILD)
    message("Skipping building of ${name}")
  else()
    # Build normally
    add_child_image_from_source(${name} ${sourcedir})
  endif()
endfunction()

function(add_child_image_from_source name sourcedir)
  message("\n=== child image ${name} begin ===")

  # Construct a list of variables that, when present in the root
  # image, should be passed on to all child images as well.
  list(APPEND
    SHARED_MULTI_IMAGE_VARIABLES
    CMAKE_BUILD_TYPE
    BOARD_DIR
    ZEPHYR_MODULES
    ZEPHYR_EXTRA_MODULES
    ZEPHYR_TOOLCHAIN_VARIANT
    GNUARMEMB_TOOLCHAIN_PATH
    EXTRA_KCONFIG_TARGETS
    )

  foreach(kconfig_target ${EXTRA_KCONFIG_TARGETS})
    list(APPEND
      SHARED_MULTI_IMAGE_VARIABLES
      EXTRA_KCONFIG_TARGET_COMMAND_FOR_${kconfig_target}
      )
  endforeach()

  unset(image_cmake_args)
  list(REMOVE_DUPLICATES SHARED_MULTI_IMAGE_VARIABLES)
  foreach(shared_var ${SHARED_MULTI_IMAGE_VARIABLES})
    if(DEFINED ${shared_var})
      list(APPEND image_cmake_args
        -D${shared_var}=${${shared_var}}
        )
    endif()
  endforeach()

  # Set ${name}_BOARD based on what BOARD is set to.
  image_board_selection(${BOARD} ${name}_BOARD)

  get_cmake_property(VARIABLES              VARIABLES)
  get_cmake_property(VARIABLES_CACHED CACHE_VARIABLES)

  set(regex "^${name}_.+")

  list(FILTER VARIABLES        INCLUDE REGEX ${regex})
  list(FILTER VARIABLES_CACHED INCLUDE REGEX ${regex})

  foreach(var_name
      ${VARIABLES}
      ${VARIABLES_CACHED}
      )
    # This regex is guaranteed to match due to the filtering done
    # above, we only re-run the regex to extract the part after
    # '_'. We run the regex twice because it is believed that
    # list(FILTER is faster than doing a string(REGEX on each item.
    string(REGEX MATCH "^${name}_(.+)" unused_out_var ${var_name})

    # When we try to pass a list on to the child image, like
    # -DCONF_FILE=a.conf;b.conf, we will get into trouble because ; is
    # a special character, so we escape it (mucho) to get the expected
    # behaviour.
    string(REPLACE \; \\\\\; val "${${var_name}}")

    list(APPEND image_cmake_args
      -D${CMAKE_MATCH_1}=${val}
      )
  endforeach()

  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${name})
  execute_process(
    COMMAND ${CMAKE_COMMAND}
    -G${CMAKE_GENERATOR}
    ${EXTRA_MULTI_IMAGE_CMAKE_ARGS} # E.g. --trace-expand
    -DIMAGE_NAME=${name}_
    ${image_cmake_args}
    ${sourcedir}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${name}
    RESULT_VARIABLE ret
    )

  set_property(DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_BINARY_DIR}/${name}/zephyr/.config
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "CMake generation for ${name} failed, aborting. Command: ${ret}")
  endif()

  message("=== child image ${name} end ===\n")

  # Include some variables from the child image into the parent image
  # namespace
  include(${CMAKE_BINARY_DIR}/${name}/shared_vars.cmake)

  # Increase the scope of this variable to make it more available
  set(${name}_KERNEL_HEX_NAME ${${name}_KERNEL_HEX_NAME} CACHE STRING "" FORCE)

  include(ExternalProject)
  ExternalProject_Add(${name}_subimage
    SOURCE_DIR ${sourcedir}
    BINARY_DIR ${CMAKE_BINARY_DIR}/${name}
    BUILD_BYPRODUCTS ${${name}_BUILD_BYPRODUCTS} # Set by shared_vars.cmake
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
    ${EXTRA_MULTI_IMAGE_BUILD_OPT} # E.g. -v
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    )

  foreach(kconfig_target
      menuconfig
      guiconfig
      ${EXTRA_KCONFIG_TARGETS}
      )
    add_custom_target(${name}_${kconfig_target}
      ${CMAKE_MAKE_PROGRAM} ${kconfig_target}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${name}
      USES_TERMINAL
      )
  endforeach()

  set_property(
    GLOBAL APPEND PROPERTY
    PM_IMAGES
    "${name}"
    )
endfunction()
