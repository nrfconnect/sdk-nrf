#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include_guard(GLOBAL)

# Usage:
#   UpdateableImage_Add(APPLICATION <name>
#                       GROUP <name>
#   )
#
# This function includes an image inside a list that will be used to trigger
# bootloader-dependent build and packaging logic.
#
# APPLICATION <name>: Name of the application.
# GROUP <name>:       Application group (i.e. VARIANT)
function(UpdateableImage_Add)
  cmake_parse_arguments(VIMAGE "" "APPLICATION;GROUP" "" ${ARGN})

  if(VIMAGE_GROUP)
    set(group ${VIMAGE_GROUP})
  else()
    set(group "DEFAULT")
  endif()

  # Update list of avilable groups
  get_property(
    groups GLOBAL PROPERTY sysbuild_updateable_groups
  )
  list(APPEND groups ${group})
  list(REMOVE_DUPLICATES groups)
  set_property(
    GLOBAL PROPERTY sysbuild_updateable_groups ${groups}
  )

  # Append the image name
  set_property(
    GLOBAL
    APPEND PROPERTY sysbuild_updateable_${group} ${VIMAGE_APPLICATION}
  )
endfunction()

# Usage:
#   UpdateableImage_Get(<outvar> [ALL|GROUP <group>])
#
# This function returns a list of images, assigned to the specified group.
#
# <outvar>:       Name of variable to set.
# ALL:            Get images from all groups.
# GROUP <group>:  Image update group.
#                 Use "DEFAULT" to get the default group (including DEFAULT_IMAGE).
function(UpdateableImage_Get outvar)
  set(all_images)
  set(group)

  cmake_parse_arguments(VGRP "ALL" "GROUP" "" ${ARGN})
  if(VGRP_GROUP)
    list(APPEND group ${VGRP_GROUP})
  endif()

  if(VGRP_ALL)
    get_property(
      group GLOBAL PROPERTY sysbuild_updateable_groups
    )
    list(APPEND group "DEFAULT")
    list(REMOVE_DUPLICATES group)
  endif()

  foreach(image_group ${group})
    get_property(
      images GLOBAL PROPERTY sysbuild_updateable_${image_group}
    )
    if(images)
      list(APPEND all_images "${images}")
    endif()

    # Include the DEFAULT_IMAGE if the DEFAULT group is used.
    if(${image_group} STREQUAL "DEFAULT")
      list(APPEND all_images ${DEFAULT_IMAGE})
    endif()
  endforeach()

  set(${outvar} "${all_images}" PARENT_SCOPE)
endfunction()

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
    -DPRELOAD_BINARY_DIR=${${VBUILD_APPLICATION}_BINARY_DIR}
  )

  # Configure variant image after application so that the configuration is present
  sysbuild_add_dependencies(CONFIGURE ${VBUILD_VARIANT} ${VBUILD_APPLICATION})
endfunction()

# Usage:
#   FirmwareUpdaterImage_Get(<outvar>)
#
# This function returns a list of images, configured as firmware loader images.
#
# <outvar>:       Name of variable to set.
function(FirmwareUpdaterImage_Get outvar)
  set(fw_loader_images)
  get_property(images GLOBAL PROPERTY sysbuild_images)

  foreach(image ${images})
    set(app_type)
    get_property(app_type TARGET ${image} PROPERTY APP_TYPE)

    if("${app_type}" STREQUAL "FIRMWARE_LOADER")
      list(APPEND fw_loader_images ${image})
    endif()
  endforeach()

  set(${outvar} "${fw_loader_images}" PARENT_SCOPE)
endfunction()
