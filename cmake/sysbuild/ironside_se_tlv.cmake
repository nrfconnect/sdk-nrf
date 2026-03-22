# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(generate_ironside_se_tlvs_image out_extra_imgtool_args)
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

function(generate_ironside_se_tlvs_sysbuild
  prefix out_extra_imgtool_args out_extra_depends image_fw_base images
)
  set(ironside_se_tlv_args)
  set(ironside_se_tlv_depends)

  foreach(image ${images})
    set(image_has_periphconf)
    set(image_periphconf_strip)
    sysbuild_get(image_has_periphconf IMAGE ${image} VAR CONFIG_NRF_PERIPHCONF_SECTION KCONFIG)
    sysbuild_get(image_periphconf_strip IMAGE ${image} VAR CONFIG_NRF_PERIPHCONF_SECTION_STRIP KCONFIG)

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

function(add_ironside_se_tlv_conf_validate_targets_merged
  prefix sign_target merged_artifact start_offset
)
  set(IRONSIDE_SUPPORT_DIR "${ZEPHYR_HAL_NORDIC_MODULE_DIR}/ironside")

  # When signing merged binaries, the PERIPHCONF TLV is generated in Sysbuild based on
  # the merged firmware.
  set(periphconf_tlv_bin ${CMAKE_BINARY_DIR}/${prefix}_periphconf_tlv.bin)
  set(IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE
    "${IRONSIDE_SUPPORT_DIR}/se/resources/periphconf_registers-nrf54h20_xxaa-v23.4.0+27.json"
  )
  set(periphconf_check_cmd
    ${CMAKE_COMMAND} -E env ZEPHYR_BASE=${ZEPHYR_BASE}
    ${PYTHON_EXECUTABLE} ${IRONSIDE_SUPPORT_DIR}/se/tool/ironside/__main__.py
    periphconf-check
    --in-periphconf-hex-mcuboot-tlv ${merged_artifact} ${periphconf_tlv_bin} ${start_offset}
    --in-periphconf-registers-json ${IRONSIDE_SE_PERIPHCONF_REGISTERS_FILE}
  )

  set(validate_stamp "${APPLICATION_BINARY_DIR}/${prefix}_periphconf_validate.stamp")
  add_custom_command(
    OUTPUT ${validate_stamp}
    COMMAND ${periphconf_check_cmd} --mode errors
    COMMAND ${CMAKE_COMMAND} -E touch ${validate_stamp}
    DEPENDS ${sign_target} ${merged_artifact} ${periphconf_tlv_bin}
    COMMENT "Validating PERIPHCONF entries in ${merged_artifact}"
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
    DEPENDS ${sign_target} ${merged_artifact} ${periphconf_tlv_bin}
    COMMENT "Printing PERIPHCONF entries in ${merged_artifact}"
  )
  add_custom_target(
    ${prefix}_periphconf_dump_raw
    COMMAND ${periphconf_check_cmd} --mode full --style raw
    DEPENDS ${sign_target} ${merged_artifact} ${periphconf_tlv_bin}
    COMMENT "Printing PERIPHCONF entries in ${merged_artifact}"
  )
endfunction()

function(add_ironside_se_tlv_conf_validate_targets_split prefix images)
  set(input_file_args)
  set(input_file_deps)

  foreach(image ${images})
    set(image_has_periphconf_tlv)
    sysbuild_get(image_has_periphconf_tlv IMAGE ${image} VAR CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV KCONFIG)
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
    endif()

    list(APPEND input_file_deps ${image})
  endforeach()

  if(input_file_args)
    set(IRONSIDE_SUPPORT_DIR "${ZEPHYR_HAL_NORDIC_MODULE_DIR}/ironside")
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

function(ironside_se_tlv_conf_post_cmake)
  if(SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
    # Merged binary signing uses add_ironside_se_tlv_conf_validate_targets_merged instead.
    return()
  endif()

  UpdateableImage_Get(default_images GROUP "DEFAULT")
  add_ironside_se_tlv_conf_validate_targets_split(mcuboot_primary "${default_images}")

  UpdateableImage_Get(variant_images GROUP "VARIANT")
  add_ironside_se_tlv_conf_validate_targets_split(mcuboot_secondary "${variant_images}")

  FirmwareUpdaterImage_Get(firmware_updater_images)
  add_ironside_se_tlv_conf_validate_targets_split(mcuboot_firmware_updater "${firmware_updater_images}")
endfunction()
