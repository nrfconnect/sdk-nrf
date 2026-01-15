#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

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

  message(WARNING "Unable to find image ${image_name} code partition inside mcuboot devicetree."
    "You can fix it by defining a slot<x>_partition that includes the address ${code_addr} inside "
    "sysbuild/mcuboot.overlay or sysbuild/mcuboot/boards/<board_name>.overlay file.")
  set(${out_var} "${out_var}-NOTFOUND" PARENT_SCOPE)
endfunction()

yaml_create(NAME mcuboot_manifest)
yaml_set(NAME mcuboot_manifest KEY format VALUE "1")
yaml_set(NAME mcuboot_manifest KEY images LIST)
set(manifest_path "manifest.yaml")
set(manifest_img_slot_0 "${DEFAULT_IMAGE}")

yaml_create(NAME mcuboot_secondary_manifest)
yaml_set(NAME mcuboot_secondary_manifest KEY format VALUE "1")
yaml_set(NAME mcuboot_secondary_manifest KEY images LIST)
set(manifest_secondary_path "manifest_secondary.yaml")
set(manifest_img_slot_1 "mcuboot_secondary_app")

# There is no need to generate a manifest if there is only a single (merged) image.
if(NOT SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
  sysbuild_get(manifest_img IMAGE mcuboot VAR CONFIG_MCUBOOT_MANIFEST_IMAGE_INDEX KCONFIG)

  UpdateableImage_Get(images GROUP "DEFAULT")
  foreach(image ${images})
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_BIN_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    find_image_index(${image} image_index)

    if("${image_index}" EQUAL "${manifest_img}")
      yaml_set(NAME mcuboot_manifest KEY manifest_index VALUE "${manifest_img}")
      cmake_path(APPEND BINARY_DIR "zephyr" "manifest.yaml" OUTPUT_VARIABLE manifest_path)
      set(manifest_img_slot_0 "${image}")
      continue()
    endif()

    if(NOT "${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
      cmake_path(APPEND BINARY_DIR "zephyr" "${BINARY_BIN_FILE}.signed.bin" OUTPUT_VARIABLE image_path)
    else()
      cmake_path(APPEND BINARY_DIR "zephyr" "${BINARY_BIN_FILE}.bin" OUTPUT_VARIABLE image_path)
    endif()

    if("${image_index}" STREQUAL "image_index-NOTFOUND")
      yaml_set(NAME mcuboot_manifest KEY images APPEND LIST MAP
        "path: ${image_path}, name: ${image}"
      )
    else()
      yaml_set(NAME mcuboot_manifest KEY images APPEND LIST MAP
        "path: ${image_path}, name: ${image}, index: ${image_index}"
      )
    endif()
  endforeach()

  foreach(image ${images})
    if("${image}" STREQUAL "${manifest_img_slot_0}")
      continue()
    endif()
    add_dependencies("${manifest_img_slot_0}" "${image}")
  endforeach()

  UpdateableImage_Get(variants GROUP "VARIANT")
  foreach(image ${variants})
    sysbuild_get(BINARY_DIR IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    sysbuild_get(BINARY_BIN_FILE IMAGE ${image} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
    find_image_index(${image} image_index)

    if("${image_index}" EQUAL "${manifest_img}")
      yaml_set(NAME mcuboot_secondary_manifest KEY manifest_index VALUE "${manifest_img}")
      cmake_path(APPEND BINARY_DIR "zephyr" "manifest.yaml" OUTPUT_VARIABLE manifest_secondary_path)
      set(manifest_img_slot_1 "${image}")
      continue()
    endif()

    if(NOT "${SB_CONFIG_SIGNATURE_TYPE}" STREQUAL "NONE")
      cmake_path(APPEND BINARY_DIR "zephyr" "${BINARY_BIN_FILE}.signed.bin" OUTPUT_VARIABLE image_path)
    else()
      cmake_path(APPEND BINARY_DIR "zephyr" "${BINARY_BIN_FILE}.bin" OUTPUT_VARIABLE image_path)
    endif()

    if("${image_index}" STREQUAL "image_index-NOTFOUND")
      yaml_set(NAME mcuboot_secondary_manifest KEY images APPEND LIST MAP
        "path: ${image_path}, name: ${image}"
      )
    else()
      yaml_set(NAME mcuboot_secondary_manifest KEY images APPEND LIST MAP
        "path: ${image_path}, name: ${image}, index: ${image_index}"
      )
    endif()
  endforeach()

  foreach(image ${variants})
    if("${image}" STREQUAL "${manifest_img_slot_1}")
      continue()
    endif()
    add_dependencies("${manifest_img_slot_1}" "${image}")
  endforeach()
endif()

yaml_save(NAME mcuboot_manifest FILE "${manifest_path}")
yaml_save(NAME mcuboot_secondary_manifest FILE "${manifest_secondary_path}")
