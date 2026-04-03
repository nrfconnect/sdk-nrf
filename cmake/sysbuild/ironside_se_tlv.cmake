# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Generate IronSide SE TLV artifacts to be included in the image metadata.
# Expects to be called from an image signing script.
function(generate_ironside_se_tlvs out_extra_imgtool_args)
  set(ironside_se_tlv_args)
  set(extra_imgtool_args)

  if(CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV)
    set(periphconf_tlv_bin ${ZEPHYR_BINARY_DIR}/periphconf_tlv.bin)
    list(APPEND ironside_se_tlv_args --periphconf-tlv-out "${periphconf_tlv_bin}")

    # The TLV contains an offset into the image data that is relative to the start
    # of the firmware.
    dt_chosen(code_partition PROPERTY "zephyr,code-partition")
    dt_partition_addr(slot_addr PATH "${code_partition}" REQUIRED ABSOLUTE)
    math(EXPR image_fw_base
      "${slot_addr} + ${CONFIG_ROM_START_OFFSET}"
      OUTPUT_FORMAT HEXADECIMAL
    )

    list(APPEND ironside_se_tlv_args --image-fw-base ${image_fw_base})
    list(APPEND extra_imgtool_args
      --custom-tlv-file
      ${CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV_ID}
      "${periphconf_tlv_bin}"
    )
  endif()

  if(ironside_se_tlv_args)
    # Note: this needs to be executed before the imgtool command that uses it
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
      COMMAND ${PYTHON_EXECUTABLE}
      "${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/ironside_se_tlv.py"
      --elf "${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}"
      ${ironside_se_tlv_args}
    )
  endif()

  set(${out_extra_imgtool_args} "${extra_imgtool_args}" PARENT_SCOPE)
endfunction()

# Generate IronSide SE TLV artifacts to be included in the image metadata, for merged images.
# Expects to be called from the merged image signing script in Sysbuild.
function(generate_ironside_se_tlvs_merged
  prefix out_extra_imgtool_args out_extra_depends image_fw_base images
)
  set(ironside_se_tlv_args)
  set(ironside_se_tlv_depends)

  foreach(image ${images})
    set(image_has_periphconf)
    set(image_periphconf_strip)
    sysbuild_get(image_has_periphconf IMAGE ${image} VAR CONFIG_NRF_PERIPHCONF_SECTION KCONFIG)
    sysbuild_get(image_periphconf_strip
      IMAGE ${image} VAR CONFIG_NRF_PERIPHCONF_SECTION_STRIP KCONFIG
    )

    # Note: CONFIG_NRF_PERIPHCONF_SECTION_STRIP=y implies that the PERIPHCONF section is included
    # in UICR instead, therefore we check both of these.
    if(image_has_periphconf AND NOT image_periphconf_strip)
      set(image_elf)
      sysbuild_get(image_elf IMAGE ${image} VAR BYPRODUCT_KERNEL_ELF_NAME CACHE)
      list(APPEND ironside_se_tlv_args --elf ${image_elf})
      list(APPEND ironside_se_tlv_depends ${image_elf})
    endif()
  endforeach()

  if(NOT ironside_se_tlv_args)
    return()
  endif()

  set(periphconf_tlv_bin ${CMAKE_BINARY_DIR}/${prefix}_periphconf_tlv.bin)

  list(APPEND ironside_se_tlv_args --image-fw-base ${image_fw_base})
  list(APPEND ironside_se_tlv_args --periphconf-tlv-out ${periphconf_tlv_bin})
  list(APPEND extra_imgtool_args
    --custom-tlv-file ${SB_CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV_ID} ${periphconf_tlv_bin}
  )

  add_custom_command(
    OUTPUT ${periphconf_tlv_bin}
    COMMAND ${PYTHON_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/ironside_se_tlv.py
    ${ironside_se_tlv_args}
    DEPENDS
    ${ironside_se_tlv_depends}
  )
  add_custom_target(
    ironside_se_tlv_${prefix}
    DEPENDS
    ${periphconf_tlv_bin}
    COMMENT "Generate IronSide SE TLVs"
  )

  string(STRIP "${extra_imgtool_args}" extra_imgtool_args)
  set(${out_extra_imgtool_args} "${extra_imgtool_args}" PARENT_SCOPE)
  set(${out_extra_depends} ironside_se_tlv_${prefix} ${periphconf_tlv_bin} PARENT_SCOPE)
endfunction()

# Add targets for validating or printing the contents of IronSide SE TLV data for a set of images
# belonging that are booted at the same time.
function(add_ironside_se_tlv_conf_validate_targets prefix images)
  set(input_file_args)
  set(input_file_deps)

  foreach(image ${images})
    set(image_has_periphconf_tlv)
    sysbuild_get(image_has_periphconf_tlv
      IMAGE ${image} VAR CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV KCONFIG
    )
    if(NOT image_has_periphconf_tlv)
      continue()
    endif()

    set(image_binary_dir)
    sysbuild_get(image_binary_dir IMAGE ${image} VAR APPLICATION_BINARY_DIR CACHE)
    set(image_periphconf_tlv_bin ${image_binary_dir}/zephyr/periphconf_tlv.bin)

    set(image_start_offset)
    sysbuild_get(image_start_offset IMAGE ${image} VAR CONFIG_ROM_START_OFFSET KCONFIG)

    set(image_signed_hex)
    sysbuild_get(image_signed_hex IMAGE ${image} VAR BYPRODUCT_KERNEL_SIGNED_HEX_NAME CACHE)
    if(NOT ("${image_signed_hex}" STREQUAL ""))
      list(APPEND input_file_args
        --in-periphconf-hex-mcuboot-tlv
        ${image_signed_hex} ${image_periphconf_tlv_bin} ${image_start_offset}
      )
      list(APPEND input_file_deps ${image_signed_hex})
    else()
      set(image_signed_bin)
      sysbuild_get(image_signed_bin IMAGE ${image} VAR BYPRODUCT_KERNEL_SIGNED_BIN_NAME CACHE)
      if("${image_signed_bin}" STREQUAL "")
        message(WARNING
          "Image ${image} has PERIPHCONF TLV but neither HEX nor BIN outputs. "
          "It will be excluded from PERIPHCONF validation and debug printing."
        )
        continue()
      endif()
      list(APPEND input_file_args
        --in-periphconf-bin-mcuboot-tlv
        ${image_signed_bin} ${image_periphconf_tlv_bin} ${image_start_offset}
      )
      list(APPEND input_file_deps ${image_signed_bin})
    endif()
  endforeach()

  if(input_file_args)
    sysbuild_get(IRONSIDE_SUPPORT_DIR IMAGE ${DEFAULT_IMAGE} VAR IRONSIDE_SUPPORT_DIR CACHE)
    set(IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE
      "${IRONSIDE_SUPPORT_DIR}/se/resources/periphconf_registers-nrf54h20_xxaa-v23.4.0+27.json"
    )

    set(periphconf_check_cmd
      ${CMAKE_COMMAND} -E env ZEPHYR_BASE=${ZEPHYR_BASE}
      ${PYTHON_EXECUTABLE} ${IRONSIDE_SUPPORT_DIR}/se/tool/ironside/__main__.py
      periphconf-check
      ${input_file_args}
      --in-periphconf-registers-json ${IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE}
    )

    string(REPLACE ";" ", " image_names "${images}")
    # File used to prevent the validate target from always being rebuilt
    set(validate_stamp "${APPLICATION_BINARY_DIR}/${prefix}_periphconf_validate.stamp")
    add_custom_command(
      OUTPUT ${validate_stamp}
      COMMAND ${periphconf_check_cmd} --mode errors
      COMMAND ${CMAKE_COMMAND} -E touch ${validate_stamp}
      DEPENDS ${input_file_deps}
      COMMENT "Validating PERIPHCONF entries in ${image_names}"
      VERBATIM
    )
    add_custom_target(
      ${prefix}_periphconf_validate
      ALL
      DEPENDS ${validate_stamp}
    )
    add_custom_target(
      ${prefix}_periphconf_dump
      COMMAND ${periphconf_check_cmd} --mode full
      DEPENDS ${input_file_deps}
      COMMENT "Printing PERIPHCONF entries in ${image_names}"
    )
    add_custom_target(
      ${prefix}_periphconf_dump_raw
      COMMAND ${periphconf_check_cmd} --mode full --style raw
      DEPENDS ${input_file_deps}
      COMMENT "Printing PERIPHCONF entries in ${image_names}"
    )
  endif()
endfunction()

# Add targets for validating or printing the contents of IronSide SE TLV data for a merged image.
function(add_ironside_se_tlv_conf_validate_targets_merged
  tlv_prefix main_image sign_target merged_artifact start_offset
)
  if(NOT SB_CONFIG_SOC_NRF54H20)
    # Only 54H has the required JSON file at the moment
    return()
  endif()

  set(target_prefix)

  # We want to have the target name be the same for merged/non-merged images.
  UpdateableImage_Get(default_images GROUP "DEFAULT")
  if(${main_image} IN_LIST default_images)
    set(target_prefix "mcuboot_primary")
  else()
    UpdateableImage_Get(variant_images GROUP "VARIANT")
    if(${main_image} IN_LIST variant_images)
      set(target_prefix "mcuboot_secondary")
    else()
      FirmwareUpdaterImage_Get(firmware_updater_images)
      if(${main_image} IN_LIST firmware_updater_images)
        set(target_prefix "mcuboot_firmware_updater")
      endif()
    endif()
  endif()

  if(NOT target_prefix)
    message(WARNING "Failed to map image ${main_image} to updateable image group")
    return()
  endif()

  sysbuild_get(IRONSIDE_SUPPORT_DIR IMAGE ${DEFAULT_IMAGE} VAR IRONSIDE_SUPPORT_DIR CACHE)
  set(IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE
    "${IRONSIDE_SUPPORT_DIR}/se/resources/periphconf_registers-nrf54h20_xxaa-v23.4.0+27.json"
  )

  # When signing merged binaries, the PERIPHCONF TLV is generated in Sysbuild based on
  # the merged firmware.
  set(periphconf_tlv_bin ${CMAKE_BINARY_DIR}/${tlv_prefix}_periphconf_tlv.bin)
  set(periphconf_check_cmd
    ${CMAKE_COMMAND} -E env ZEPHYR_BASE=${ZEPHYR_BASE}
    ${PYTHON_EXECUTABLE} ${IRONSIDE_SUPPORT_DIR}/se/tool/ironside/__main__.py
    periphconf-check
    --in-periphconf-hex-mcuboot-tlv ${merged_artifact} ${periphconf_tlv_bin} ${start_offset}
    --in-periphconf-registers-json ${IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE}
  )

  # File used to prevent the validate target from always being rebuilt
  set(validate_stamp "${APPLICATION_BINARY_DIR}/${target_prefix}_periphconf_validate.stamp")
  add_custom_command(
    OUTPUT ${validate_stamp}
    COMMAND ${periphconf_check_cmd} --mode errors
    COMMAND ${CMAKE_COMMAND} -E touch ${validate_stamp}
    DEPENDS ${sign_target} ${merged_artifact} ${periphconf_tlv_bin}
    COMMENT "Validating PERIPHCONF entries in ${merged_artifact}"
    VERBATIM
  )
  add_custom_target(
    ${target_prefix}_periphconf_validate
    ALL
    DEPENDS ${validate_stamp}
  )
  add_custom_target(
    ${target_prefix}_periphconf_dump
    COMMAND ${periphconf_check_cmd} --mode full
    DEPENDS ${sign_target} ${merged_artifact} ${periphconf_tlv_bin}
    COMMENT "Printing PERIPHCONF entries in ${merged_artifact}"
  )
  add_custom_target(
    ${target_prefix}_periphconf_dump_raw
    COMMAND ${periphconf_check_cmd} --mode full --style raw
    DEPENDS ${sign_target} ${merged_artifact} ${periphconf_tlv_bin}
    COMMENT "Printing PERIPHCONF entries in ${merged_artifact}"
  )
endfunction()

# Function intended to be called from the module post_cmake hook.
# A function is used to prevent this from being run when the file is included in other contexts.
function(ironside_se_tlv_conf_post_cmake)
  if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
    # Merged binary signing uses add_ironside_se_tlv_conf_validate_targets_merged instead.
    return()
  endif()

  UpdateableImage_Get(default_images GROUP "DEFAULT")
  add_ironside_se_tlv_conf_validate_targets(mcuboot_primary "${default_images}")

  UpdateableImage_Get(variant_images GROUP "VARIANT")
  add_ironside_se_tlv_conf_validate_targets(mcuboot_secondary "${variant_images}")

  FirmwareUpdaterImage_Get(firmware_updater_images)
  add_ironside_se_tlv_conf_validate_targets(
    mcuboot_firmware_updater
    "${firmware_updater_images}"
  )
endfunction()
