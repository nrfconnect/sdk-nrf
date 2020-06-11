#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

function(share content)
  # Adds 'content' as a line in the 'shared_vars' property.
  # This property is again written to a file which is imported as a cmake file
  # by the parent image. In other words, this function can be used to share
  # information (variables, lists etc) with the parent image.
  #
  # Example usage 'share("set(visible_in_parent \"I AM YOUR CHILD\")")'

  set_property(
    TARGET         zephyr_property_target
    APPEND_STRING
    PROPERTY       shared_vars
    "${content}\n"
    )
endfunction()

if(IMAGE_NAME)
  share("set(${IMAGE_NAME}KERNEL_HEX_NAME ${KERNEL_HEX_NAME})")
  share("set(${IMAGE_NAME}ZEPHYR_BINARY_DIR ${ZEPHYR_BINARY_DIR})")
  # Share the elf file, in order to support symbol loading for debuggers.
  share("set(${IMAGE_NAME}KERNEL_ELF_NAME ${KERNEL_ELF_NAME})")
  share("list(APPEND ${IMAGE_NAME}BUILD_BYPRODUCTS ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME})")
  share("list(APPEND ${IMAGE_NAME}BUILD_BYPRODUCTS ${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME})")

  file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/shared_vars.cmake
    CONTENT $<TARGET_PROPERTY:zephyr_property_target,shared_vars>
    )
endif(IMAGE_NAME)

function(add_child_image)
  set(oneValueArgs NAME SOURCE_DIR DOMAIN)
  cmake_parse_arguments(ACI "" "${oneValueArgs}" "" ${ARGN})

  if (NOT ACI_NAME OR NOT ACI_SOURCE_DIR)
    message(FATAL_ERROR "Missing parameter, required: NAME SOURCE_DIR")
  endif()

  string(TOUPPER ${ACI_NAME} UPNAME)

  if (CONFIG_${UPNAME}_BUILD_STRATEGY_USE_HEX_FILE)
    assert_exists(CONFIG_${UPNAME}_HEX_FILE)
    message("Using ${CONFIG_${UPNAME}_HEX_FILE} instead of building ${ACI_NAME}")

    # Set property so that the hex file is merged in by partition manager.
    set_property(GLOBAL PROPERTY ${ACI_NAME}_PM_HEX_FILE ${CONFIG_${UPNAME}_HEX_FILE})
  elseif (CONFIG_${UPNAME}_BUILD_STRATEGY_SKIP_BUILD)
    message("Skipping building of ${ACI_NAME}")
  else()
    # Build normally
    add_child_image_from_source(${ARGN})
  endif()
endfunction()

# See 'add_child_image'
function(add_child_image_from_source)
  set(oneValueArgs NAME SOURCE_DIR DOMAIN)
  cmake_parse_arguments(ACI "" "${oneValueArgs}" "" ${ARGN})

  if (NOT ACI_NAME OR NOT ACI_SOURCE_DIR)
    message(FATAL_ERROR "Missing parameter, required: NAME SOURCE_DIR")
  endif()


  # Add the new partition manager domain if needed.
  # The domain corresponds to the BOARD without the 'ns' suffix.
  if (ACI_DOMAIN)
    if (${ACI_NAME}_BOARD)
      message(FATAL_ERROR
        "Parent image cannot set BOARD of the child image when DOMAIN is set")
    endif()
    set(${ACI_NAME}_BOARD ${ACI_DOMAIN})
    set(${ACI_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION ${ACI_NAME} CACHE STRING "" FORCE)
  elseif (NOT ${ACI_NAME}_BOARD)
    # Set ${ACI_NAME}_BOARD based on what BOARD is set to if not already set by parent
    # It is assumed that only the root app will be built as non-secure.
    # This is not a valid assumption as there might be multiple non-secure
    # images defined.
    # TODO: Allow multiple non-secure images by using Kconfig to set the
    # secure/non-secure property rather than using a separate board definition.
    get_board_without_ns_suffix(${BOARD} ${ACI_NAME}_BOARD)
  endif()

  if (NOT (${${ACI_NAME}_BOARD} IN_LIST PM_DOMAINS))
    list(APPEND PM_DOMAINS ${${ACI_NAME}_BOARD})
    share("list(APPEND PM_DOMAINS ${${ACI_NAME}_BOARD})")
  endif()


  message("\n=== child image ${ACI_NAME} - ${${ACI_NAME}_BOARD} begin ===")
  # Construct a list of variables that, when present in the root
  # image, should be passed on to all child images as well.
  list(APPEND
    SHARED_MULTI_IMAGE_VARIABLES
    CMAKE_BUILD_TYPE
    CMAKE_VERBOSE_MAKEFILE
    BOARD_DIR
    ZEPHYR_MODULES
    ZEPHYR_EXTRA_MODULES
    ZEPHYR_TOOLCHAIN_VARIANT
    GNUARMEMB_TOOLCHAIN_PATH
    EXTRA_KCONFIG_TARGETS
    NCS_TOOLCHAIN_VERSION
    PM_DOMAINS
    ${ACI_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION
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

  get_cmake_property(VARIABLES              VARIABLES)
  get_cmake_property(VARIABLES_CACHED CACHE_VARIABLES)

  set(regex "^${ACI_NAME}_.+")

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
    string(REGEX MATCH "^${ACI_NAME}_(.+)" unused_out_var ${var_name})

    # When we try to pass a list on to the child image, like
    # -DCONF_FILE=a.conf;b.conf, we will get into trouble because ; is
    # a special character, so we escape it (mucho) to get the expected
    # behaviour.
    string(REPLACE \; \\\\\; val "${${var_name}}")

    list(APPEND image_cmake_args
      -D${CMAKE_MATCH_1}=${val}
      )
  endforeach()

  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${ACI_NAME})
  execute_process(
    COMMAND ${CMAKE_COMMAND}
    -G${CMAKE_GENERATOR}
    ${EXTRA_MULTI_IMAGE_CMAKE_ARGS} # E.g. --trace-expand
    -DIMAGE_NAME=${ACI_NAME}_
    ${image_cmake_args}
    ${ACI_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${ACI_NAME}
    RESULT_VARIABLE ret
    )

  if (IMAGE_NAME)
    # Expose your childrens secrets to your parent
    share("include(${CMAKE_BINARY_DIR}/${ACI_NAME}/shared_vars.cmake)")
  endif()

  set_property(DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_BINARY_DIR}/${ACI_NAME}/zephyr/.config
    )

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "CMake generation for ${ACI_NAME} failed, aborting. Command: ${ret}")
  endif()

  message("=== child image ${ACI_NAME} - ${${ACI_NAME}_BOARD} end ===\n")

  # Include some variables from the child image into the parent image
  # namespace
  include(${CMAKE_BINARY_DIR}/${ACI_NAME}/shared_vars.cmake)

  # Increase the scope of this variable to make it more available
  set(${ACI_NAME}_KERNEL_HEX_NAME ${${ACI_NAME}_KERNEL_HEX_NAME} CACHE STRING "" FORCE)
  set(${ACI_NAME}_KERNEL_ELF_NAME ${${ACI_NAME}_KERNEL_ELF_NAME} CACHE STRING "" FORCE)
  set(PM_DOMAINS ${PM_DOMAINS} CACHE STRING "" FORCE)

  if(MULTI_IMAGE_DEBUG_MAKEFILE AND "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    set(multi_image_build_args "-d" "${MULTI_IMAGE_DEBUG_MAKEFILE}")
  endif()
  if(MULTI_IMAGE_DEBUG_MAKEFILE AND "${CMAKE_GENERATOR}" STREQUAL "Unix Makefiles")
    set(multi_image_build_args "--debug=${MULTI_IMAGE_DEBUG_MAKEFILE}")
  endif()

  include(ExternalProject)
  ExternalProject_Add(${ACI_NAME}_subimage
    SOURCE_DIR ${ACI_SOURCE_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}/${ACI_NAME}
    BUILD_BYPRODUCTS ${${ACI_NAME}_BUILD_BYPRODUCTS} # Set by shared_vars.cmake
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} --build . -- ${multi_image_build_args}
    INSTALL_COMMAND ""
    BUILD_ALWAYS True
    USES_TERMINAL_BUILD True
    )

  foreach(kconfig_target
      menuconfig
      guiconfig
      ${EXTRA_KCONFIG_TARGETS}
      )

    add_custom_target(${ACI_NAME}_${kconfig_target}
      ${CMAKE_MAKE_PROGRAM} ${kconfig_target}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${ACI_NAME}
      USES_TERMINAL
      )
  endforeach()

  if (NOT "${ACI_NAME}" STREQUAL "${${ACI_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION}")
    set_property(
      GLOBAL APPEND PROPERTY
      PM_IMAGES
      "${ACI_NAME}"
      )
  endif()

  if (ACI_DOMAIN)
    add_custom_target(${ACI_NAME}_flash
                      COMMAND
                      ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${ACI_NAME}
                      --target flash
                      DEPENDS
                      ${ACI_NAME}_subimage
    )

    set_property(TARGET zephyr_property_target
                 APPEND PROPERTY FLASH_DEPENDENCIES
                 ${ACI_NAME}_flash
  )
  endif()

endfunction()
