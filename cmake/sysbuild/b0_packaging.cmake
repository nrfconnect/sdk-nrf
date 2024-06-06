# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# B0 will boot the app directly, create the DFU zip with both update candidates
include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

sysbuild_get(app_fw_info_firmware_version IMAGE ${DEFAULT_IMAGE} VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)

set(s0_name "signed_by_b0_${DEFAULT_IMAGE}.bin")
set(s1_name signed_by_b0_s1_image.bin)

generate_dfu_zip(
  OUTPUT ${CMAKE_BINARY_DIR}/dfu_application.zip
  BIN_FILES ${CMAKE_BINARY_DIR}/signed_by_b0_${DEFAULT_IMAGE}.bin ${CMAKE_BINARY_DIR}/signed_by_b0_s1_image.bin
  ZIP_NAMES ${s0_name} ${s1_name}
  TYPE application
  IMAGE ${DEFAULT_IMAGE}
  SCRIPT_PARAMS
  "${s0_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_S0_ADDRESS>"
  "${s0_name}image_index=0"
  "${s0_name}slot=0"
  "${s0_name}version_B0=${app_fw_info_firmware_version}"
  "${s1_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_S1_ADDRESS>"
  "${s1_name}image_index=0"
  "${s1_name}slot=1"
  "${s1_name}version_B0=${app_fw_info_firmware_version}"
  DEPENDS
  ${DEFAULT_IMAGE}_extra_byproducts
  ${DEFAULT_IMAGE}_signed_kernel_hex_target
  s1_image_extra_byproducts
  s1_image_signed_kernel_hex_target
  )
