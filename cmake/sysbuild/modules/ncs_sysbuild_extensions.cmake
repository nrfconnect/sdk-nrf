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
# SPLIT_KCONFIG:      Flag indicating that the variant image should
#                     not reuse the same .config files.
function(ExternalNcsVariantProject_Add)
  cmake_parse_arguments(VBUILD "SPLIT_KCONFIG" "APPLICATION;VARIANT" "" ${ARGN})

  ExternalProject_Get_Property(${VBUILD_APPLICATION} SOURCE_DIR BINARY_DIR)
  set(${VBUILD_APPLICATION}_BINARY_DIR ${BINARY_DIR})

  get_property(VARIANT_BOARD TARGET ${VBUILD_APPLICATION} PROPERTY BOARD)
  if(DEFINED VARIANT_BOARD)
    ExternalZephyrProject_Add(
      APPLICATION ${VBUILD_VARIANT}
      SOURCE_DIR ${SOURCE_DIR}
      BOARD ${VARIANT_BOARD}
      BUILD_ONLY true
  )
  else()
    ExternalZephyrProject_Add(
      APPLICATION ${VBUILD_VARIANT}
      SOURCE_DIR ${SOURCE_DIR}
      BUILD_ONLY true
    )
  endif()

  set_property(TARGET ${VBUILD_VARIANT} PROPERTY NCS_VARIANT_APPLICATION ${VBUILD_APPLICATION})
  set_property(TARGET ${VBUILD_VARIANT} APPEND PROPERTY _EP_CMAKE_ARGS
    -DCONFIG_NCS_IS_VARIANT_IMAGE=y
  )

  if(NOT VBUILD_SPLIT_KCONFIG)
    set_property(TARGET ${VBUILD_VARIANT} APPEND PROPERTY _EP_CMAKE_ARGS
      -DPRELOAD_BINARY_DIR=${${VBUILD_APPLICATION}_BINARY_DIR}
      -DCONFIG_NCS_VARIANT_MERGE_KCONFIG=y
    )
  endif()

  # Configure variant image after application so that the configuration is present
  sysbuild_add_dependencies(CONFIGURE ${VBUILD_VARIANT} ${VBUILD_APPLICATION})
endfunction()
