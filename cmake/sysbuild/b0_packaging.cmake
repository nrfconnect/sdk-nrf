# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(dfu_app_b0_zip_package)
  # B0 will boot the app directly, create the DFU zip with both update candidates
  include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

  sysbuild_get(app_fw_info_firmware_version IMAGE ${DEFAULT_IMAGE} VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)

  set(s0_name "signed_by_b0_${DEFAULT_IMAGE}.bin")
  b0_image_name(s1_image_name)
  set(s1_name signed_by_b0_${s1_image_name}.bin)

  if(SB_CONFIG_PARTITION_MANAGER)
    generate_dfu_zip(
      OUTPUT ${CMAKE_BINARY_DIR}/dfu_application.zip
      BIN_FILES
        ${CMAKE_BINARY_DIR}/signed_by_b0_${DEFAULT_IMAGE}.bin
        ${CMAKE_BINARY_DIR}/signed_by_b0_${s1_image_name}.bin
      ZIP_NAMES
        ${s0_name}
        ${s1_name}
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
  else()
    dt_chosen(s0_image_code_partition_path PROPERTY "zephyr,code-partition" TARGET ${DEFAULT_IMAGE})
    dt_partition_addr(s0_image_slot_address PATH "${s0_image_code_partition_path}" TARGET ${DEFAULT_IMAGE} REQUIRED)
    dt_chosen(s1_image_code_partition_path PROPERTY "zephyr,code-partition" TARGET ${s1_image_name})
    dt_partition_addr(s1_image_slot_address PATH "${s1_image_code_partition_path}" TARGET ${s1_image_name} REQUIRED)

    generate_dfu_zip(
      OUTPUT ${CMAKE_BINARY_DIR}/dfu_application.zip
      BIN_FILES
        ${CMAKE_BINARY_DIR}/signed_by_b0_${DEFAULT_IMAGE}.bin
        ${CMAKE_BINARY_DIR}/signed_by_b0_${s1_image_name}.bin
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
        ${s1_image_name}_extra_byproducts
        ${s1_image_name}_signed_kernel_hex_target
    )
  endif()
endfunction()

if(SB_CONFIG_DFU_ZIP_APP)
  dfu_app_b0_zip_package()
endif()
