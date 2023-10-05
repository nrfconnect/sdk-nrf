#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include_guard(GLOBAL)

# Usage:
#   ExternalNcsVariantProject_Add(APPLICATION <name>
#                                 VARIANT <name>
#   )
#
# This function includes a variant build of an existing Zephyr based build
# system into the multiimage build system
#
# APPLICATION <name>: Name of the application which is used as the source for
#                     the variant build.
# VARIANT <name>:     Name of the variant build.
function(ExternalNcsVariantProject_Add)
  cmake_parse_arguments(VBUILD "" "APPLICATION;VARIANT" "" ${ARGN})

  ExternalProject_Get_Property(${VBUILD_APPLICATION} SOURCE_DIR BINARY_DIR)
  set(${VBUILD_APPLICATION}_BINARY_DIR ${BINARY_DIR})

  ExternalZephyrProject_Add(
    APPLICATION ${VBUILD_VARIANT}
    SOURCE_DIR ${SOURCE_DIR}
    BUILD_ONLY true
  )

  get_cmake_property(sysbuild_cache CACHE_VARIABLES)
  foreach(var_name ${sysbuild_cache})
    if("${var_name}" MATCHES "^(${}_.*)$")
      string(LENGTH "${VBUILD_APPLICATION}" tmplen)
      string(SUBSTRING "${var_name}" ${tmplen} -1 tmp)
      set(${VBUILD_VARIANT}${tmp} "${${var_name}}" CACHE UNINITIALIZED "" FORCE)
    endif()
  endforeach()

  ExternalProject_Get_Property(${VBUILD_VARIANT} BINARY_DIR)
  ExternalProject_Add_Step(${VBUILD_VARIANT} variant_config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${${VBUILD_APPLICATION}_BINARY_DIR}/zephyr/.config
            ${BINARY_DIR}/zephyr/.config
    DEPENDERS build
    ALWAYS True
  )

  # Disable menuconfig and friends in variant builds by substitution with a
  # dummy target that does nothing except returning successfully.
  set_property(TARGET ${VBUILD_VARIANT} APPEND PROPERTY _EP_CMAKE_ARGS
    -DKCONFIG_TARGETS=variant_config
    "-DEXTRA_KCONFIG_TARGET_COMMAND_FOR_variant_config=-c\;''"
  )
endfunction()
