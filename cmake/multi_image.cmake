#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

function(image_board_selection board_in board_out)
  # Unless parent was "ns" in which case we assume that the child images are
  # all secure, and should be build using the secure version of the board.
  # It is assumed that only the root app will be built as non-secure.
  # This is not a valid assumption as there might be multiple non-secure
  # images defined. With this technique, it is not possible to have multiple
  # images defined as non-secure.
  # TODO: Allow multiple non-secure images by using Kconfig to set the
  # secure/non-secure property rather than using a separate board definition.
  set(${board_out} ${board_in} PARENT_SCOPE)
  if(${board_in} STREQUAL nrf9160_pca10090ns)
    set(${board_out} nrf9160_pca10090 PARENT_SCOPE)
    message("Changed board to secure nrf9160_pca10090 (NOT NS)")
  endif()

  if(${BOARD} STREQUAL nrf9160_pca20035ns)
    set(${board_out} nrf9160_pca20035 PARENT_SCOPE)
    message("Changed board to secure nrf9160_pca20035 (NOT NS)")
  endif()

  if(${BOARD} STREQUAL actinius_icarus_ns)
    set(BOARD actinius_icarus)
    message("Changed board to secure actinius_icarus (NOT NS)")
  endif()
endfunction()

function(image_menuconfig target_out command_out)
  if(EXTRA_KCONFIG_TARGETS)
    set(${target_out}  "-DEXTRA_KCONFIG_TARGETS=${EXTRA_KCONFIG_TARGETS}" PARENT_SCOPE)
    set(${command_out} "-DEXTRA_KCONFIG_TARGET_COMMAND_FOR_${EXTRA_KCONFIG_TARGETS}=${EXTRA_KCONFIG_TARGET_COMMAND_FOR_${EXTRA_KCONFIG_TARGETS}}" PARENT_SCOPE)
  endif()
endfunction()

function(zephyr_add_external_image name sourcedir)
  string(TOUPPER ${name} UPNAME)

  if (CONFIG_${UPNAME}_BUILD_STRATEGY_USE_HEX_FILE)
    assert_exists(CONFIG_${UPNAME}_HEX_FILE)
    message("Using ${CONFIG_${UPNAME}_HEX_FILE} instead of building ${name}")
  elseif (CONFIG_${UPNAME}_BUILD_STRATEGY_SKIP_BUILD)
    message("Skipping building of ${name}")
  else()
    # Build normally
    zephyr_add_external_image_from_source(${name} ${sourcedir})
  endif()
endfunction()

function(zephyr_add_external_image_from_source name sourcedir)
  string(TOUPPER ${name} UPNAME)
  set(${UPNAME}_CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/${name})
  set(${UPNAME}_PROJECT_BINARY_DIR ${CMAKE_BINARY_DIR}/${name}/zephyr)

  get_cmake_property(VARIABLES_CACHED CACHE_VARIABLES)
  get_cmake_property(VARIABLES              VARIABLES)

  unset(image_cmake_args)
  foreach(var_name
      ${VARIABLES}
      ${VARIABLES_CACHED}
      )
    if("${var_name}" MATCHES "^${name}_(.+)") # TODO: How expensive?
      list(APPEND image_cmake_args
        -D${CMAKE_MATCH_1}=${${var_name}}
        )
    endif()
  endforeach()

  file(MAKE_DIRECTORY ${${UPNAME}_CMAKE_BINARY_DIR})
  message("=== child image ${name} begin ===")
  image_board_selection(${BOARD} IMAGE_BOARD)
  image_menuconfig(IMAGE_KCONFIG_TARGET IMAGE_COMMAND_TARGET)
  execute_process(
    COMMAND ${CMAKE_COMMAND}
    # Add other toolchain here,
    -G${CMAKE_GENERATOR}
    -DBOARD=${IMAGE_BOARD}
    ${IMAGE_KCONFIG_TARGET}
    ${IMAGE_COMMAND_TARGET}
    -DIS_CHILD_IMAGE=True
    ${sourcedir}
    ${image_cmake_args}
    WORKING_DIRECTORY ${${UPNAME}_CMAKE_BINARY_DIR}
    RESULT_VARIABLE ret
    )
  set(${UPNAME}_DOTCONFIG ${${UPNAME}_PROJECT_BINARY_DIR}/.config)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${${UPNAME}_DOTCONFIG})

  if(NOT ${ret} EQUAL "0")
    message(FATAL_ERROR "CMake generation for ${name} failed, aborting. Command: ${ret}")
  endif()

  message("=== child image ${name} end ===")

  # The variables inhereted from PARENT_SCOPE might be overwritten when additional Kconfig is loaded locally.
  # Therefore, safe guard the variables from parent scope that are needed.
  set(APP_CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS ${CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS})
  import_kconfig(CONFIG_ ${${UPNAME}_PROJECT_BINARY_DIR}/.config)

  # Clear it, just in case it was defined in parent scope
  set(spm_veneers_lib)
  if(CONFIG_${UPNAME}_CONTAINS_SECURE_ENTRIES)
    set(spm_veneers_lib ${${UPNAME}_CMAKE_BINARY_DIR}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME})
  endif()

  define_property(GLOBAL PROPERTY ${name}_KERNEL_NAME
                 BRIEF_DOCS "Name (KERNEL_NAME) for the SPM image."
                 FULL_DOCS "To be used to access e.g. this image's hex file."
                )
  set_property(GLOBAL PROPERTY ${name}_KERNEL_NAME ${CONFIG_KERNEL_BIN_NAME})

  include(ExternalProject)
  ExternalProject_Add(${UPNAME}_subimage
      SOURCE_DIR ${sourcedir}
      BINARY_DIR ${${UPNAME}_CMAKE_BINARY_DIR}
      BUILD_BYPRODUCTS "${spm_veneers_lib}"
                        ${${UPNAME}_PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.hex
      # The CMake configure has been executed above due to cross
      # dependencies between the multiple CMake configure stages.
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
      INSTALL_COMMAND ""
      BUILD_ALWAYS True
    )

    # ToDo: Consider to use a dedicated target and populate that target directly
    # with the files as names and deps.
    # That would allow to cleanup logic found in partition_manager.cmake

    foreach(kconfig_target
        menuconfig
        guiconfig
        ${EXTRA_KCONFIG_TARGETS}
        )
        add_custom_target(${name}_${kconfig_target} ${CMAKE_MAKE_PROGRAM} ${kconfig_target}
                          WORKING_DIRECTORY ${${UPNAME}_CMAKE_BINARY_DIR}
                          USES_TERMINAL
                         )
    endforeach()

    if (APP_CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS AND spm_veneers_lib)
      # Link the entry veneers library file with the Non-Secure Firmware that needs it.
      zephyr_link_libraries(${spm_veneers_lib})

      # Add dependency for the benefit of make.
      add_dependencies(zephyr_interface ${spm_veneers_lib})
    endif()

  set_property(
    GLOBAL APPEND PROPERTY
    PM_IMAGES
    "${name}"
    )
endfunction()
