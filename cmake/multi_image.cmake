#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(IMAGE_NAME)
  set_shared(IMAGE ${IMAGE_NAME} PROPERTY KERNEL_HEX_NAME ${KERNEL_HEX_NAME})
  set_shared(IMAGE ${IMAGE_NAME} PROPERTY ZEPHYR_BINARY_DIR ${ZEPHYR_BINARY_DIR})
  # Share the elf file, in order to support symbol loading for debuggers.
  set_shared(IMAGE ${IMAGE_NAME} PROPERTY KERNEL_ELF_NAME ${KERNEL_ELF_NAME})
  set_shared(IMAGE ${IMAGE_NAME}
    PROPERTY BUILD_BYPRODUCTS
             ${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
             ${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}
  )
  # Share the signing key file so that the parent image can use it to
  # generate signed update candidates.
  if(CONFIG_BOOT_SIGNATURE_KEY_FILE)
    set_shared(IMAGE ${IMAGE_NAME} PROPERTY SIGNATURE_KEY_FILE ${CONFIG_BOOT_SIGNATURE_KEY_FILE})
  endif()

  generate_shared(IMAGE ${IMAGE_NAME} FILE ${CMAKE_BINARY_DIR}/shared_vars.cmake)
endif(IMAGE_NAME)

#
# Function for gathering all CMake arguments relevant to Kconfig and Devicetree 
# for a child image. The output can be passed directly to a CMake call.
#
# Usage:
#   get_child_image_cmake_args(
#     CHILD_IMAGE            <name of child image>
#     OUTPUT_CMAKE_ARGS      <output variable for storing cmake args>
#   )
function(get_child_image_cmake_args)

  set(oneValueArgs CHILD_IMAGE CONFIG_OUT)
  cmake_parse_arguments(GET_ARGS "" "${oneValueArgs}" "" ${ARGN})

  if (NOT GET_ARGS_CHILD_IMAGE OR NOT GET_ARGS_CONFIG_OUT)
    message(FATAL_ERROR "Missing parameter, required: ${oneValueArgs}")
  endif()

  # It is possible for a sample to use a custom set of Kconfig fragments for a
  # child image, or to append additional Kconfig fragments to the child image.
  # Note that <CHILD_IMAGE> in this context is the name of the child image as
  # passed to the 'add_child_image' function.
  #
  # <child-sample> DIRECTORY
  # | - prj.conf (A)
  # | - prj_<buildtype>.conf (B)
  # | - boards DIRECTORY
  # | | - <board>.conf (C)
  # | | - <board>_<buildtype>.conf (D)


  # <current-sample> DIRECTORY
  # | - prj.conf
  # | - prj_<buildtype>.conf
  # | - child_image DIRECTORY
  #     |-- <CHILD_IMAGE>.conf (I)                 Fragment, used together with (A) and (C)
  #     |-- <CHILD_IMAGE>_<buildtype>.conf (J)     Fragment, used together with (B) and (D)
  #     |-- <CHILD_IMAGE>.overlay                  If present, will be merged with BOARD.dts
  #     |-- <CHILD_IMAGE> DIRECTORY
  #         |-- boards DIRECTORY
  #         |   |-- <board>.conf (E)            If present, use instead of (C), requires (G).
  #         |   |-- <board>_<buildtype>.conf (F)     If present, use instead of (D), requires (H).
  #         |   |-- <board>.overlay             If present, will be merged with BOARD.dts
  #         |   |-- <board>_<revision>.overlay  If present, will be merged with BOARD.dts
  #         |-- prj.conf (G)                    If present, use instead of (A)
  #         |                                   Note that (C) is ignored if this is present.
  #         |                                   Use (E) instead.
  #         |-- prj_<buildtype>.conf (H)        If present, used instead of (B) when user
  #         |                                   specify `-DCONF_FILE=prj_<buildtype>.conf for
  #         |                                   parent image. Note that any (C) is ignored
  #         |                                   if this is present. Use (F) instead.
  #         |-- <board>.overlay                 If present, will be merged with BOARD.dts
  #         |-- <board>_<revision>.overlay      If present, will be merged with BOARD.dts
  #
  # Note: The folder `child_image/<CHILD_IMAGE>` is only need when configurations
  #       files must be used instead of the child image default configs.
  #       The append a child image default config, place the addetional settings
  #       in `child_image/<CHILD_IMAGE>.conf`.
  set(fragments_dir ${APPLICATION_CONFIG_DIR}/child_image)
  set(conf_dir ${APPLICATION_CONFIG_DIR}/child_image/${GET_ARGS_CHILD_IMAGE})
  if (NOT ${GET_ARGS_CHILD_IMAGE}_CONF_FILE)
    ncs_file(CONF_FILES ${conf_dir}
             BOARD ${ACI_BOARD}
             # Child image always uses the same revision as parent board.
             BOARD_REVISION ${BOARD_REVISION}
             KCONF ${GET_ARGS_CHILD_IMAGE}_CONF_FILE
             DTS ${GET_ARGS_CHILD_IMAGE}_DTC_OVERLAY_FILE
             BUILD ${CONF_FILE_BUILD_TYPE}
    )

    # Check for configuration fragment. The contents of these are appended
    # to the project configuration, as opposed to the CONF_FILE which is used
    # as the base configuration.
    if (DEFINED CONF_FILE_BUILD_TYPE)
      set(child_image_conf_fragment ${fragments_dir}/${GET_ARGS_CHILD_IMAGE}_${CONF_FILE_BUILD_TYPE}.conf)
    else()
      set(child_image_conf_fragment ${fragments_dir}/${GET_ARGS_CHILD_IMAGE}.conf)
    endif()
    if (EXISTS ${child_image_conf_fragment})
      add_overlay_config(${GET_ARGS_CHILD_IMAGE} ${child_image_conf_fragment})
    endif()

    # Check for overlay named <GET_ARGS_CHILD_IMAGE>.overlay.
    set(child_image_dts_overlay ${fragments_dir}/${GET_ARGS_CHILD_IMAGE}.overlay)
    if (EXISTS ${child_image_dts_overlay})
      add_overlay_dts(${GET_ARGS_CHILD_IMAGE} ${child_image_dts_overlay})
    endif()
    if(DEFINED ${GET_ARGS_CHILD_IMAGE}_CONF_FILE)
      set(${GET_ARGS_CHILD_IMAGE}_CONF_FILE ${${GET_ARGS_CHILD_IMAGE}_CONF_FILE} CACHE STRING
        "Default ${GET_ARGS_CHILD_IMAGE} configuration file"
      )
    endif()
  endif()

  # Construct a list of variables that, when present in the root
  # image, should be passed on to all child images as well.
  list(APPEND
    SHARED_MULTI_IMAGE_VARIABLES
    CMAKE_BUILD_TYPE
    CMAKE_VERBOSE_MAKEFILE
    BOARD_DIR
    BOARD_REVISION
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

  unset(child_image_cmake_args)
  list(REMOVE_DUPLICATES SHARED_MULTI_IMAGE_VARIABLES)
  foreach(shared_var ${SHARED_MULTI_IMAGE_VARIABLES})
    if(DEFINED ${shared_var})
      # Any  shared var that is a list must be escaped to ensure correct behaviour.
      string(REPLACE \; \\\\\; val "${${shared_var}}")
      list(APPEND child_image_cmake_args
        -D${shared_var}=${val}
        )
    endif()
  endforeach()

  get_cmake_property(VARIABLES              VARIABLES)
  get_cmake_property(VARIABLES_CACHED CACHE_VARIABLES)

  set(regex "^${GET_ARGS_CHILD_IMAGE}_.+")

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
    string(REGEX MATCH "^${GET_ARGS_CHILD_IMAGE}_(.+)" unused_out_var ${var_name})

    # When we try to pass a list on to the child image, like
    # -DCONF_FILE=a.conf;b.conf, we will get into trouble because ; is
    # a special character, so we escape it (mucho) to get the expected
    # behaviour.
    string(REPLACE \; \\\\\; val "${${var_name}}")

    list(APPEND child_image_cmake_args
      -D${CMAKE_MATCH_1}=${val}
      )
  endforeach()

  unset(${GET_ARGS_CONFIG_OUT} PARENT_SCOPE)
  set(${GET_ARGS_CONFIG_OUT} "${child_image_cmake_args}" PARENT_SCOPE)
endfunction()

function(add_s1_variant)
  set(oneValueArgs BASE_IMAGE SOURCE_DIR)
  cmake_parse_arguments(ASV "" "${oneValueArgs}" "" ${ARGN})

  set(s1_override_conf
    ${ZEPHYR_NRF_MODULE_DIR}/subsys/bootloader/image/is_s1_image.conf)

  add_image_variant(
    IMAGE_NAME s1_image
    BASE_IMAGE ${ASV_BASE_IMAGE}
    EXTRA_CONFIG ${s1_override_conf}
    SOURCE_DIR ${ASV_SOURCE_DIR}
    )
endfunction()

# Adds a variant of an image to the build.
# The configuration passed to the variant image is a copy of that sent to the
# base image, except what is passed to the EXTRA_CONFIG argument.
#
# Required arguments are:
# IMAGE_NAME - The name of the variant image
# BASE_IMAGE - The name of the base image which the variant image is based on.
# SOURCE_DIR - The source dir of the base image.
# EXTRA_CONFIG - The extra configuration fragment to pass to the variant image.
function(add_image_variant)

  set(oneValueArgs BASE_IMAGE IMAGE_NAME SOURCE_DIR EXTRA_CONFIG)
  cmake_parse_arguments(AIV "" "${oneValueArgs}" "" ${ARGN})

  if (AIV_BASE_IMAGE)
    # A base image has been specified, this means that we need to copy the
    # configuration of a child image (as opposed to copying the configuration
    # from the top level parent image (that is, 'app').

    set(base_image_configuration "${${AIV_BASE_IMAGE}_IMAGE_CMAKE_ARGS}")
  else()
    # No base image is specified, this means that the base image is the top
    # level parent image (that is, 'app'). Copy relevant information from the
    # 'app' image CMakeCache in order to build an identical variant image

    get_cmake_property(variables_cached CACHE_VARIABLES)
    foreach(var_name ${variables_cached})
      # If '-DCONF_FILE' is specified, it is unset by boilerplate.cmake and
      # replaced with 'CACHED_CONF_FILE' in the cache. Therefore we need this
      # special handling for passing the value to the variant image.
      if("${var_name}" MATCHES "CACHED_CONF_FILE")
          list(APPEND application_vars ${var_name})
      endif()

      if("${var_name}" MATCHES "^CONFIG_.*")
          list(APPEND application_vars ${var_name})
      endif()

      get_property(var_help CACHE ${var_name} PROPERTY HELPSTRING)
      if("${var_help}" STREQUAL "No help, variable specified on the command line.")
        list(APPEND application_vars ${var_name})
      endif()
    endforeach()

    foreach(app_var_name ${application_vars})
      get_property(var_type  CACHE ${app_var_name} PROPERTY TYPE)
      list(APPEND base_image_configuration "-D${var_name}:${var_type}=${${app_var_name}}")
    endforeach()
  endif()

  # Pass the extra configuration fragment to variant image.
  if (base_image_configuration MATCHES ".*OVERLAY_CONFIG")
    string(REGEX REPLACE
      "-DOVERLAY_CONFIG="
      "-DOVERLAY_CONFIG=${AIV_EXTRA_CONFIG}\\\\;"
      variant_image_cmake_args
      "${base_image_configuration}"
      )
  else()
    set(variant_image_cmake_args "${base_image_configuration}")
    list(APPEND variant_image_cmake_args "-DOVERLAY_CONFIG=${AIV_EXTRA_CONFIG}")
  endif()

  # Place the image name at the end to avoid confusion with variables that are
  # automatically passed to a child image since it is prefixed with the child
  # image name.
  set(PRE_DEFINED_IMAGE_CMAKE_ARGS_${AIV_IMAGE_NAME} "${variant_image_cmake_args}" CACHE INTERNAL "")

  add_child_image(
    NAME ${AIV_IMAGE_NAME}
    SOURCE_DIR ${AIV_SOURCE_DIR}
    )
endfunction()

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

  if (CONFIG_IS_S1_IMAGE)
    # Don't add child images while building the S1 variant image.
    return()
  endif()

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

    set(domain_parent ${${ACI_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION})
    if(DEFINED ${ACI_DOMAIN}_PM_DOMAIN_DYNAMIC_PARTITION
       AND NOT "${domain_parent}" STREQUAL "${ACI_NAME}"
    )
      # A domain may only have one child image, which can then act as a parent
      # to other images in the domain.
      # As it is a cache variable we check it's content so that CMake re-run
      # will pass the check as long as the child image hasn't changed.
      message(FATAL_ERROR "A domain may only have a single child image."
        "Current domain image is: ${domain_parent}, `${domain_parent}` is a "
        "domain parent image, so you may add `${ACI_NAME}` as a child inside "
        "`${domain_parent}`"
      )
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

  unset(image_cmake_args)
  if (PRE_DEFINED_IMAGE_CMAKE_ARGS_${ACI_NAME})
    # A complete set of configurations has been specified. Use this instead of
    # finding the cmake args the normal way.
    set(image_cmake_args "${PRE_DEFINED_IMAGE_CMAKE_ARGS_${ACI_NAME}}")
  else()
    get_child_image_cmake_args(
      CHILD_IMAGE ${ACI_NAME}
      CONFIG_OUT image_cmake_args
      )
  endif()

  # Expose the CMake args for the current image in case we want to create a variant build
  set(${ACI_NAME}_IMAGE_CMAKE_ARGS "${image_cmake_args}" CACHE INTERNAL "")

  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${ACI_NAME})
  execute_process(
    COMMAND ${CMAKE_COMMAND}
    -G${CMAKE_GENERATOR}
    ${EXTRA_MULTI_IMAGE_CMAKE_ARGS} # E.g. --trace-expand
    -DIMAGE_NAME=${ACI_NAME}
    ${image_cmake_args}
    ${ACI_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${ACI_NAME}
    RESULT_VARIABLE ret
    )

  if (IMAGE_NAME)
    # Expose your childrens secrets to your parent
    set_shared(FILE ${CMAKE_BINARY_DIR}/${ACI_NAME}/shared_vars.cmake)
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

  if(MULTI_IMAGE_DEBUG_MAKEFILE AND "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    set(multi_image_build_args "-d" "${MULTI_IMAGE_DEBUG_MAKEFILE}")
  endif()
  if(MULTI_IMAGE_DEBUG_MAKEFILE AND "${CMAKE_GENERATOR}" STREQUAL "Unix Makefiles")
    set(multi_image_build_args "--debug=${MULTI_IMAGE_DEBUG_MAKEFILE}")
  endif()

  get_shared(${ACI_NAME}_byproducts IMAGE ${ACI_NAME} PROPERTY BUILD_BYPRODUCTS)

  include(ExternalProject)
  ExternalProject_Add(${ACI_NAME}_subimage
    SOURCE_DIR ${ACI_SOURCE_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}/${ACI_NAME}
    BUILD_BYPRODUCTS ${${ACI_NAME}_byproducts}
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
