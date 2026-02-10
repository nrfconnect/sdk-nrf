#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

#
# Find the image index based on the sysbuild image name by comparing the code
# partition address with the mcuboot slot partitions.
#
# Usage:
#   find_image_index(<image_name> <out_var>)
#
# Parameters:
#   image_name - Name of the sysbuild image to find the index for.
#   out_var    - Output variable to store the found image index.
#                If not found, will be set to "<out_var>-NOTFOUND".
#
# Notes:
#   This function relies on the devicetree properties set for mcuboot and the
#   chosen code partition. Ensure that the devicetree overlays are correctly
#   configured to reflect the partitioning scheme.
function(find_image_index image_name out_var)
  dt_chosen(code_flash TARGET ${image_name} PROPERTY "zephyr,code-partition")
  dt_partition_addr(code_addr PATH "${code_flash}" TARGET ${image_name} ABSOLUTE REQUIRED)

  foreach(image_index RANGE 0 ${SB_CONFIG_MCUBOOT_UPDATEABLE_IMAGES})
    math(EXPR slot_0 "${image_index} * 2")
    math(EXPR slot_1 "${image_index} * 2 + 1")

    dt_partition_addr(slot0_addr LABEL "slot${slot_0}_partition" TARGET mcuboot ABSOLUTE)
    dt_partition_addr(slot1_addr LABEL "slot${slot_1}_partition" TARGET mcuboot ABSOLUTE)

    if(("${code_addr}" STREQUAL "${slot0_addr}") OR ("${code_addr}" STREQUAL "${slot1_addr}"))
      set(${out_var} "${image_index}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  set(${out_var} "${out_var}-NOTFOUND" PARENT_SCOPE)
endfunction()

#
# Generate MCUboot manifests for all updateable images for a given group.
#
# Usage:
#   generate_mcuboot_manifests(<manifest_file_name> <image_group>)
#
# Parameters:
#   manifest_file_name - Name of the manifest file to generate (e.g. mcuboot_manifest.yaml).
#   image_group        - Group of updateable images to include in the manifest
#                        (e.g. DEFAULT, VARIANT).
function(generate_mcuboot_manifest manifest_name image_group)
  # Generate default mcuboot manifests directly inside the build directory.
  set(manifest_dir "${CMAKE_BINARY_DIR}")

  # Read the manifest image index from MCUboot image configuration
  sysbuild_get(manifest_idx IMAGE mcuboot VAR CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX KCONFIG)
  if("${image_group}" STREQUAL "DEFAULT")
    math(EXPR manifest_slot "${manifest_idx} * 2")
  else()
    math(EXPR manifest_slot "${manifest_idx} * 2 + 1")
  endif()

  # Primary image(s) manifest
  set(mcuboot_yaml_ctx mcuboot_manifest_${image_group})
  yaml_create(NAME ${mcuboot_yaml_ctx})
  yaml_set(NAME ${mcuboot_yaml_ctx} KEY format VALUE "1")
  yaml_set(NAME ${mcuboot_yaml_ctx} KEY manifest_index VALUE "${manifest_idx}")
  yaml_set(NAME ${mcuboot_yaml_ctx} KEY images LIST)
  cmake_path(APPEND manifest_dir "${manifest_name}" OUTPUT_VARIABLE manifest_path)

  # Include all updateable images
  UpdateableImage_Get(images GROUP "${image_group}")

  # Find the manifest image among the updateable images
  set(manifest_image)
  foreach(image ${images})
    find_image_index(${image} image_index)
    if("${image_index}" EQUAL "${manifest_idx}")
      set(manifest_image "${image}")
      break()
    endif()
  endforeach()

  if("${manifest_image}" STREQUAL "")
    message(WARNING "MCUboot manifest generation failed. "
      "Unable to find the manifest image compiled for slot${manifest_slot}_partition "
      "among updateable images. "
      "You can fix it by calling UpdateableImage_Add(<image_name>) on the manifest image."
    )
    return()
  endif()

  # If a custom manifest is used, do not automatically generate manifests.
  sysbuild_get(manifest_config_dir IMAGE ${manifest_image} VAR APPLICATION_CONFIG_DIR CACHE)
  cmake_path(APPEND manifest_config_dir "${manifest_name}" OUTPUT_VARIABLE custom_manifest)
  if(EXISTS "${custom_manifest}")
    message(STATUS "Custom manifest file found for ${image_group} group at ${custom_manifest}. "
      "Skipping automatic generation of mcuboot manifest.")
  else()
    message(STATUS "Generating MCUboot manifest for ${image_group} group at "
      "${manifest_dir}/${manifest_name}"
    )
    # If not found, continue with the manifest generation.
    foreach(image ${images})
      sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
      sysbuild_get(BINARY_BIN_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
      find_image_index(${image} image_index)

      if("${image_index}" EQUAL "${manifest_idx}")
        continue()
      endif()

      if(NOT "${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
        cmake_path(APPEND BINARY_DIR "zephyr" "${BINARY_BIN_FILE}.signed.bin" OUTPUT_VARIABLE
          image_path
        )
      else()
        cmake_path(APPEND BINARY_DIR "zephyr" "${BINARY_BIN_FILE}.bin" OUTPUT_VARIABLE image_path)
      endif()

      if("${image_index}" STREQUAL "image_index-NOTFOUND")
        message(WARNING "MCUboot manifest generation failed. "
          "Unable to find image ${image} code partition inside mcuboot devicetree. "
          "You can fix it by providing a custom ${manifest_name} configuration file or by "
          "defining a slot<x>_partition that includes the code partition address inside "
          "sysbuild/mcuboot.overlay or sysbuild/mcuboot/boards/<board_name>.overlay file.")
        return()
      else()
        yaml_set(NAME ${mcuboot_yaml_ctx} KEY images APPEND LIST MAP
          "path: ${image_path}, name: ${image}, index: ${image_index}"
        )
      endif()
    endforeach()

    yaml_save(NAME ${mcuboot_yaml_ctx} FILE "${manifest_path}")
  endif()

  foreach(image ${images})
    if("${image}" STREQUAL "${manifest_image}")
      continue()
    endif()
    add_dependencies("${manifest_image}" "${image}")
  endforeach()
endfunction()

# There is no need to generate a manifest if there is only a single (merged) image.
if(NOT SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
  generate_mcuboot_manifest("mcuboot_manifest.yaml" "DEFAULT")
  generate_mcuboot_manifest("mcuboot_manifest_secondary.yaml" "VARIANT")
endif()
