# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(dfu_app_b0_zip_package)
  # B0 will boot the app directly, create the DFU zip with both update candidates
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

  sysbuild_get(app_fw_info_firmware_version IMAGE ${DEFAULT_IMAGE} VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)

  set(s0_name "signed_by_b0_${DEFAULT_IMAGE}.bin")
  set(s1_name signed_by_b0_${DEFAULT_IMAGE}_s1_variant.bin)

  dt_chosen(s0_image_code_partition_path PROPERTY "zephyr,code-partition" TARGET ${DEFAULT_IMAGE})
  dt_partition_addr(s0_image_slot_address PATH "${s0_image_code_partition_path}" TARGET ${DEFAULT_IMAGE} REQUIRED)
  dt_chosen(s1_image_code_partition_path PROPERTY "zephyr,code-partition" TARGET ${DEFAULT_IMAGE}_s1_variant)
  dt_partition_addr(s1_image_slot_address PATH "${s1_image_code_partition_path}" TARGET ${DEFAULT_IMAGE}_s1_variant REQUIRED)

  generate_dfu_zip(
    OUTPUT ${CMAKE_BINARY_DIR}/dfu_application.zip
    BIN_FILES
      ${CMAKE_BINARY_DIR}/signed_by_b0_${DEFAULT_IMAGE}.bin
      ${CMAKE_BINARY_DIR}/signed_by_b0_${DEFAULT_IMAGE}_s1_variant.bin
    ZIP_NAMES
      ${s0_name}
      ${s1_name}
    TYPE application
    IMAGE ${DEFAULT_IMAGE}
    SCRIPT_PARAMS
      "${s0_name}load_address=${s0_image_slot_address}"
      "${s0_name}image_index=0"
      "${s0_name}slot=0"
      "${s0_name}version_B0=${app_fw_info_firmware_version}"
      "${s1_name}load_address=${s1_image_slot_address}"
      "${s1_name}image_index=0"
      "${s1_name}slot=1"
      "${s1_name}version_B0=${app_fw_info_firmware_version}"
    DEPENDS
      ${DEFAULT_IMAGE}_extra_byproducts
      ${DEFAULT_IMAGE}_signed_kernel_hex_target
      ${DEFAULT_IMAGE}_s1_variant_extra_byproducts
      ${DEFAULT_IMAGE}_s1_variant_signed_kernel_hex_target
  )
endfunction()

if(SB_CONFIG_DFU_ZIP_APP)
  dfu_app_b0_zip_package()
endif()
