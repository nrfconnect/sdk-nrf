# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(NOT SB_CONFIG_BOOTLOADER_MCUBOOT)
  return()
endif()

if(NOT SB_CONFIG_MCUBOOT_EXTRA_IMAGES OR SB_CONFIG_MCUBOOT_EXTRA_IMAGES EQUAL 0)
  return()
endif()

get_property(extra_paths GLOBAL PROPERTY DFU_EXTRA_PATHS)
if(NOT extra_paths)
  return()
endif()

set(keyfile "${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}")
set(keyfile_enc "${SB_CONFIG_BOOT_ENCRYPTION_KEY_FILE}")
string(CONFIGURE "${keyfile}" keyfile)
string(CONFIGURE "${keyfile_enc}" keyfile_enc)

if(NOT "${SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE}")
  if("${keyfile}" STREQUAL "")
    message(WARNING "Neither SB_CONFIG_BOOT_SIGNATURE_TYPE_NONE or "
                    "SB_CONFIG_BOOT_SIGNATURE_KEY_FILE are set, the generated build will not be "
                    "bootable by MCUboot unless it is signed manually/externally.")
    return()
  endif()
endif()

if(NOT DEFINED IMGTOOL)
  message(WARNING "Can't sign images for MCUboot: can't find imgtool. To fix, install imgtool with pip3, or add the mcuboot repository to the west manifest and ensure it has a scripts/imgtool.py file.")
  return()
endif()

# Fetch devicetree details for flash and slot information
dt_chosen(flash_node TARGET ${DEFAULT_IMAGE} PROPERTY "zephyr,flash")
dt_nodelabel(slot0_flash TARGET ${DEFAULT_IMAGE} NODELABEL "slot0_partition" REQUIRED)
dt_prop(slot_size TARGET ${DEFAULT_IMAGE} PATH "${slot0_flash}" PROPERTY "reg" INDEX 1 REQUIRED)
dt_prop(write_block_size TARGET ${DEFAULT_IMAGE} PATH "${flash_node}" PROPERTY "write-block-size")

if(NOT write_block_size)
  set(write_block_size 4)
  message(WARNING "slot0_partition write block size devicetree parameter is missing, assuming write block size is 4")
endif()

get_property(extra_names GLOBAL PROPERTY DFU_EXTRA_NAMES)
get_property(extra_versions GLOBAL PROPERTY DFU_EXTRA_VERSIONS)
get_property(extra_targets GLOBAL PROPERTY DFU_EXTRA_TARGETS)

# Iterate through all extra images and sign them
list(LENGTH extra_paths num_binaries)
if(num_binaries GREATER 0)
  math(EXPR last_index "${num_binaries} - 1")

  set(signed_paths "")
  set(signed_names "")

  foreach(index RANGE ${last_index})
    math(EXPR slot_number "${index} + 1")

    if(NOT DEFINED NCS_MCUBOOT_EXTRA_${slot_number}_IMAGE_NUMBER OR NCS_MCUBOOT_EXTRA_${slot_number}_IMAGE_NUMBER EQUAL -1)
      message(WARNING "Extra image slot ${slot_number} not configured. Set SB_CONFIG_MCUBOOT_EXTRA_IMAGES to at least ${num_binaries} in sysbuild.")
      return()
    endif()

    set(image_id ${NCS_MCUBOOT_EXTRA_${slot_number}_IMAGE_NUMBER})

    list(GET extra_paths ${index} input_bin)
    list(GET extra_names ${index} image_name)
    list(GET extra_versions ${index} version)

    if(NOT image_name OR image_name STREQUAL "")
      get_filename_component(image_name "${input_bin}" NAME_WE)
    endif()

    set(output_bin "${CMAKE_BINARY_DIR}/${image_name}.signed.bin")
    set(output_hex "${CMAKE_BINARY_DIR}/${image_name}.signed.hex")

    set(imgtool_sign
      ${PYTHON_EXECUTABLE} ${IMGTOOL} sign
      --version ${version}
      --align ${write_block_size}
      --slot-size $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_${image_id}_SIZE>
      --pad-header
      --header-size ${SB_CONFIG_PM_MCUBOOT_PAD}
    )

    set(imgtool_extra "")
    if(NOT "${keyfile}" STREQUAL "")
      list(APPEND imgtool_extra -k "${keyfile}")
    endif()

    if(SB_CONFIG_BOOT_SIGNATURE_TYPE_PURE)
      list(APPEND imgtool_extra --pure)
    elseif(SB_CONFIG_BOOT_IMG_HASH_ALG_SHA512)
      list(APPEND imgtool_extra --sha 512)
    endif()

    if(SB_CONFIG_MCUBOOT_HARDWARE_DOWNGRADE_PREVENTION)
      list(APPEND imgtool_extra --security-counter ${SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE})
    endif()

    add_custom_command(
      OUTPUT ${output_bin}
      COMMAND ${imgtool_sign} ${imgtool_extra} ${input_bin} ${output_bin}
      DEPENDS ${input_bin}
    )

    add_custom_command(
      OUTPUT ${output_hex}
      COMMAND ${PYTHON_EXECUTABLE}
        -c "import sys; import intelhex; intelhex.bin2hex(sys.argv[1], sys.argv[2], int(sys.argv[3], 16) + int(sys.argv[4], 16)) "
        ${output_bin}
        ${output_hex}
        $<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_${image_id}_ADDRESS>
        $<IF:$<STREQUAL:$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_${image_id}_REGION>,external_flash>,${SB_CONFIG_PM_EXTERNAL_FLASH_LOGICAL_ADDR},0>
      DEPENDS ${output_bin}
      COMMENT "Converting extra image ${slot_number} (ID ${image_id}) to hex"
      VERBATIM
    )

    add_custom_target(extra_img_${slot_number}_target ALL DEPENDS ${output_hex})

    set_property(GLOBAL PROPERTY ${image_name}_PM_HEX_FILE ${output_hex})
    set_property(GLOBAL PROPERTY ${image_name}_PM_TARGET extra_img_${slot_number}_target)

    # Add signed binary path and name to lists (for DFU packaging)
    list(APPEND signed_paths "${output_bin}")
    list(APPEND signed_names "${image_name}.signed.bin")
  endforeach()

  # Update DFU_EXTRA_PATHS and DFU_EXTRA_NAMES to point to signed binaries
  set_property(GLOBAL PROPERTY DFU_EXTRA_PATHS "${signed_paths}")
  set_property(GLOBAL PROPERTY DFU_EXTRA_NAMES "${signed_names}")
endif()
