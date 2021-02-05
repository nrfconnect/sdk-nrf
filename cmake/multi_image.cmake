#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
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
  # Share the signing key file so that the parent image can use it to
  # generate signed update candidates.
  if(CONFIG_BOOT_SIGNATURE_KEY_FILE)
   share("set(${IMAGE_NAME}SIGNATURE_KEY_FILE ${CONFIG_BOOT_SIGNATURE_KEY_FILE})")
  endif()

  file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/shared_vars.cmake
    CONTENT $<TARGET_PROPERTY:zephyr_property_target,shared_vars>
    )
endif(IMAGE_NAME)


function(add_child_image)
  # Adds a child image to the build.
  #
  # Required arguments are:
  # NAME - The name of the child image
  # SOURCE_DIR - The source dir of the child image
  #
  # Optional arguments are:
  # DOMAIN - The domain to place the child image in.
  #
  # Depending on the value of CONFIG_${NAME}_BUILD_STRATEGY the child image
  # is either built from source, included as a hex file, or ignored.
  #
  # See chapter "Multi-image builds" in the documentation for more details.

  set(oneValueArgs NAME SOURCE_DIR DOMAIN)
  cmake_parse_arguments(ACI "" "${oneValueArgs}" "" ${ARGN})

  if (NOT ACI_NAME OR NOT ACI_SOURCE_DIR)
    message(FATAL_ERROR "Missing parameter, required: NAME SOURCE_DIR")
  endif()

  if (NOT CONFIG_PARTITION_MANAGER_ENABLED)
    message(FATAL_ERROR
      "CONFIG_PARTITION_MANAGER_ENABLED was not set for image ${ACI_NAME}."
      "This option must be set for an image to support being added as a child"
      "image through 'add_child_image'. This is typically done by invoking the"
      " `build_strategy` kconfig template for the child image.")
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

function(add_child_image_from_source)
  # See 'add_child_image'
  set(oneValueArgs NAME SOURCE_DIR DOMAIN BOARD)
  cmake_parse_arguments(ACI "" "${oneValueArgs}" "" ${ARGN})

  if (NOT ACI_NAME OR NOT ACI_SOURCE_DIR)
    message(FATAL_ERROR "Missing parameter, required: NAME SOURCE_DIR")
  endif()

  # Pass information that the partition manager is enabled to Kconfig.
  add_overlay_config(
    ${ACI_NAME}
    ${NRF_DIR}/subsys/partition_manager/partition_manager_enabled.conf
    )

  if (${ACI_NAME}_BOARD)
    message(FATAL_ERROR
      "${ACI_NAME}_BOARD set in outer scope. Will be ignored, use "
      "`add_child_image(BOARD ${${ACI_NAME}_BOARD} ...)` for adding a child "
      "image for specific board")
  endif()

  # Add the new partition manager domain if needed.
  # The domain corresponds to the BOARD without the 'ns' suffix.
  if (ACI_DOMAIN)
    if ("${ACI_BOARD}" STREQUAL "")
      message(FATAL_ERROR
        "No board specified for domain '${ACI_DOMAIN}'. This configuration is "
        "typically defined in ${BOARD_DIR}/Kconfig")
    endif()

    # This needs to be made globally available as it is used in other files.
    set(${ACI_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION ${ACI_NAME} CACHE INTERNAL "")

    if (NOT (${ACI_DOMAIN} IN_LIST PM_DOMAINS))
      list(APPEND PM_DOMAINS ${ACI_DOMAIN})
      set_property(GLOBAL APPEND PROPERTY PM_DOMAINS ${ACI_DOMAIN})
    endif()
  elseif (NOT ACI_BOARD)
    # No BOARD is given as argument, this triggers automatic conversion of
    # *.ns board from parent image.
    get_board_without_ns_suffix(${BOARD} ACI_BOARD)
  endif()

  if (NOT ACI_DOMAIN AND DOMAIN)
    # If no domain is specified, a child image will inherit the domain of
    # its parent.
    set(ACI_DOMAIN ${DOMAIN})
    set(inherited " (inherited)")
  endif()

  set(${ACI_NAME}_DOMAIN ${ACI_DOMAIN})
  set(${ACI_NAME}_BOARD ${ACI_BOARD})

  message("\n=== child image ${ACI_NAME} - ${ACI_DOMAIN}${inherited} begin ===")

  # It is possible for a sample to use a custom set of Kconfig fragments for a
  # child image, or to append additional Kconfig fragments to the child image.
  # Note that <ACI_NAME> in this context is the name of the child image as
  # passed to the 'add_child_image' function.
  #
  # <child-sample> DIRECTORY
  # | - prj.conf (A)
  # | - prj_<desc>.conf (B)
  # | - boards DIRECTORY
  # | | - <board>.conf (C)
  # | | - <board>_<desc>.conf (D)


  # <current-sample> DIRECTORY
  # | - prj.conf
  # | - prj_<desc>.conf
  # | - child_image DIRECTORY
  #     |-- <ACI_NAME>.conf (I)             Fragment, used together with (A) and (C)
  #     |-- <ACI_NAME>_<desc>.conf (J)      Fragment, used together with (B) and (D)
  #     |-- <ACI_NAME> DIRECTORY
  #         |-- boards DIRECTORY
  #         |   |-- <board>.conf (E)        If present, use instead of (C), requires (G).
  #         |   |-- <board>_<desc>.conf (F) If present, use instead of (D), requires (H).
  #         |-- prj.conf (G)                If present, use instead of (A)
  #         |                               Note that (C) is ignored if this is present.
  #         |                               Use (E) instead.
  #         |-- prj_<desc>.conf (H)         If present, used instead of (B) when user
  #                                         specify `-DCONF_FILE=prj_<desc>.conf for
  #                                         parent image. Note that any (C) is ignored
  #                                         if this is present. Use (F) instead.
  #
  # Note: The folder `child_image/<ACI_NAME>` is only need when configurations
  #       files must be used instead of the child image default configs.
  #       The append a child image default config, place the addetional settings
  #       in `child_image/<ACI_NAME>.conf`.
  set(ACI_CONF_DIR ${APPLICATION_SOURCE_DIR}/child_image)
  set(ACI_NAME_CONF_DIR ${APPLICATION_SOURCE_DIR}/child_image/${ACI_NAME})
  if (NOT ${ACI_NAME}_CONF_FILE)
    ncs_file(CONF_FILES ${ACI_NAME_CONF_DIR}
             BOARD ${ACI_BOARD}
             KCONF ${ACI_NAME}_CONF_FILE
             BUILD ${CONF_FILE_BUILD_TYPE}
    )

    # Check for configuration fragment. The contents of these are appended
    # to the project configuration, as opposed to the CONF_FILE which is used
    # as the base configuration.
    if (DEFINED CONF_FILE_BUILD_TYPE)
      set(child_image_conf_fragment ${ACI_CONF_DIR}/${ACI_NAME}_${CONF_FILE_BUILD_TYPE}.conf)
    else()
      set(child_image_conf_fragment ${ACI_CONF_DIR}/${ACI_NAME}.conf)
    endif()
    if (EXISTS ${child_image_conf_fragment})
      add_overlay_config(${ACI_NAME} ${child_image_conf_fragment})
    endif()
  endif()

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

  message("=== child image ${ACI_NAME} - ${ACI_DOMAIN}${inherited} end ===\n")

  # Include some variables from the child image into the parent image
  # namespace
  include(${CMAKE_BINARY_DIR}/${ACI_NAME}/shared_vars.cmake)

  # Increase the scope of this variable to make it more available
  set(${ACI_NAME}_KERNEL_HEX_NAME ${${ACI_NAME}_KERNEL_HEX_NAME} CACHE STRING "" FORCE)
  set(${ACI_NAME}_KERNEL_ELF_NAME ${${ACI_NAME}_KERNEL_ELF_NAME} CACHE STRING "" FORCE)

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

endfunction()
