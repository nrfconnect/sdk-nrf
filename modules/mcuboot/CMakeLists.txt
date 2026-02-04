add_subdirectory(${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/zephyr
                 ${CMAKE_CURRENT_BINARY_DIR}/boot/bootutil/zephyr
)

set(ironside_se_tlv_args)

if(NOT ("${CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV_BIN_FILE}" STREQUAL ""))
  list(APPEND ironside_se_tlv_args
    --periphconf-tlv-out "${CONFIG_NCS_MCUBOOT_PERIPHCONF_TLV_BIN_FILE}"
  )

  # The TLV contains an offset into the image data that is relative to the start
  # of the firmware.
  math(EXPR image_fw_base
    "${CONFIG_FLASH_BASE_ADDRESS} + ${CONFIG_FLASH_LOAD_OFFSET} + ${CONFIG_ROM_START_OFFSET}"
    OUTPUT_FORMAT HEXADECIMAL
  )

  list(APPEND ironside_se_tlv_args --image-fw-base ${image_fw_base})
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
