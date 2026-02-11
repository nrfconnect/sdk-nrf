# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include_guard(GLOBAL)

# This file acts a wrapper around another MCUboot signing script.
# The wrapper is used to extend the imgtool arguments slightly without
# defining a full custom signing script.

function(image_signing_invoke_wrapped_script)
  # Locate the actual signing script, using the same strategy as the Zephyr build
  # system does:
  #
  # 1. Sysbuild (if set) using WRAPPED_SIGNING_SCRIPT, since SIGNING_SCRIPT points to this script.
  # 2. SIGNING_SCRIPT target property (if set)
  # 3. Default MCUboot signing script
  zephyr_get(WRAPPED_SIGNING_SCRIPT SYSBUILD)

  if(NOT WRAPPED_SIGNING_SCRIPT)
    get_target_property(WRAPPED_SIGNING_SCRIPT zephyr_property_target SIGNING_SCRIPT)

    if(NOT WRAPPED_SIGNING_SCRIPT)
      set(WRAPPED_SIGNING_SCRIPT ${ZEPHYR_BASE}/cmake/mcuboot.cmake)
    endif()
  endif()

  set(imgtool_tlv_args)
  set(ironside_se_tlv_args)

  if(CONFIG_NRF_PERIPHCONF_SECTION_KEEP)
    set(periphconf_tlv_bin ${ZEPHYR_BINARY_DIR}/periphconf_tlv.bin)
    list(APPEND ironside_se_tlv_args --periphconf-tlv-out ${periphconf_tlv_bin})
    math(EXPR image_fw_base
      "${CONFIG_FLASH_BASE_ADDRESS} + ${CONFIG_FLASH_LOAD_OFFSET} + ${CONFIG_ROM_START_OFFSET}"
      OUTPUT_FORMAT HEXADECIMAL
    )
    list(APPEND ironside_se_tlv_args --image-fw-base ${image_fw_base})
    set(imgtool_periphconf_tlv_args
      "--custom-tlv-file ${CONFIG_MCUBOOT_NRF_PERIPHCONF_TLV_ID} ${periphconf_tlv_bin}"
    )
    set(imgtool_tlv_args "${imgtool_tlv_args} ${imgtool_periphconf_tlv_args}")
  endif()

  if(ironside_se_tlv_args)
    # Note: this needs to be executed before the imgtool command that uses it
    set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
                 ${PYTHON_EXECUTABLE}
                 ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/ironside_se_tlv.py
                 --elf ${BYPRODUCT_KERNEL_ELF_NAME}
                 ${ironside_se_tlv_args}
    )
  endif()

  string(STRIP "${imgtool_tlv_args}" imgtool_tlv_args)

  if(imgtool_tlv_args)
    # This config is supported for adding arbitrary imgtool arguments by the base Zephyr
    # MCUboot signing script as well as the alternate signing scripts in NCS.
    # The modification here should only be visible inside the signing script.
    if(NOT CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS STREQUAL "")
      set(CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS
        "${CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS} ${imgtool_tlv_args}"
      )
    else()
      set(CONFIG_MCUBOOT_EXTRA_IMGTOOL_ARGS "${imgtool_tlv_args}")
    endif()
  endif()

  # Include the actual signing script
  include(${WRAPPED_SIGNING_SCRIPT})
endfunction()

image_signing_invoke_wrapped_script()
