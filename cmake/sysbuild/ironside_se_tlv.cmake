# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(generate_ironside_se_tlvs out_extra_imgtool_args)
  set(ironside_se_tlv_args)
  set(extra_imgtool_args)

  if(CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV)
    set(periphconf_tlv_bin ${ZEPHYR_BINARY_DIR}/periphconf_tlv.bin)
    list(APPEND ironside_se_tlv_args --periphconf-tlv-out "${periphconf_tlv_bin}")
    # The TLV contains an offset into the image data that is relative to the start
    # of the firmware.
    math(EXPR image_fw_base
      "${CONFIG_FLASH_BASE_ADDRESS} + ${CONFIG_FLASH_LOAD_OFFSET} + ${CONFIG_ROM_START_OFFSET}"
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
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                 ${PYTHON_EXECUTABLE}
                 ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/ironside_se_tlv.py
                 --elf ${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}
                 ${ironside_se_tlv_args}
    )
  endif()

  set(${out_extra_imgtool_args} "${extra_imgtool_args}" PARENT_SCOPE)
endfunction()
