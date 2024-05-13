# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(dfu_app_zip_package)
  set(bin_files)
  set(zip_names)
  set(signed_targets)
  set(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION)
  set(exclude_files)
  set(include_files)

  sysbuild_get(CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION IMAGE ${DEFAULT_IMAGE} VAR CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG)
  sysbuild_get(CONFIG_KERNEL_BIN_NAME IMAGE ${DEFAULT_IMAGE} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)

  if(SB_CONFIG_DFU_ZIP_APP)
    if(NOT SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
      # Application
      set(generate_script_app_params
          "${DEFAULT_IMAGE}.binload_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
          "${DEFAULT_IMAGE}.binimage_index=0"
          "${DEFAULT_IMAGE}.binslot_index_primary=1"
          "${DEFAULT_IMAGE}.binslot_index_secondary=2"
          "${DEFAULT_IMAGE}.binversion_MCUBOOT=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
         )
      list(APPEND bin_files "${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin")
      list(APPEND zip_names "${DEFAULT_IMAGE}.bin")
      list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts)
      set(exclude_files EXCLUDE ${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin)
      set(include_files INCLUDE ${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin)
    else()
      # Application in DirectXIP mode
      set(generate_script_app_params
          "${DEFAULT_IMAGE}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_APP_ADDRESS>"
          "${DEFAULT_IMAGE}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
          "${DEFAULT_IMAGE}slot=0"
          "mcuboot_secondary_app.binload_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_SECONDARY_APP_ADDRESS>"
          "mcuboot_secondary_app.binslot=1"
          "mcuboot_secondary_app.binversion_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
          "load_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
          "version_MCUBOOT=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
         )

      list(APPEND bin_files "${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin;mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin")
      list(APPEND zip_names "${DEFAULT_IMAGE}.bin;mcuboot_secondary_app.bin")
      list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts mcuboot_secondary_app_extra_byproducts)
      set(exclude_files EXCLUDE ${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin;mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin)
      set(include_files INCLUDE ${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin;mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin)
    endif()
  endif()

  if(SB_CONFIG_DFU_ZIP_NET)
    # Network core
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_CPUNET)
    sysbuild_get(net_core_board IMAGE ${image_name} VAR BOARD CACHE)
    set(generate_script_app_params
        ${generate_script_app_params}
        "${image_name}.binimage_index=1"
        "${image_name}.binslot_index_primary=3"
        "${image_name}.binslot_index_secondary=4"
        "${image_name}.binload_address=$<TARGET_PROPERTY:partition_manager,CPUNET_PM_APP_ADDRESS>"
        "${image_name}.binboard=${net_core_board}"
#        "${image_name}.binversion=${net_core_version}"
        "${image_name}.binsoc=${SB_CONFIG_SOC}"
       )
    list(APPEND bin_files "signed_by_mcuboot_and_b0_${image_name}.bin")
    list(APPEND zip_names "${image_name}.bin")
    list(APPEND signed_targets ${image_name}_extra_byproducts ${image_name}_signed_packaged_target)
  endif()

  if(SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH)
    # nRF7x Wifi patch
    if(SB_CONFIG_NETCORE_APP_UPDATE)
      list(APPEND generate_script_app_params
           "nrf70.binimage_index=2"
           "nrf70.binslot_index_primary=5"
           "nrf70.binslot_index_secondary=6"
      )
    else()
      list(APPEND generate_script_app_params
           "nrf70.binimage_index=1"
           "nrf70.binslot_index_primary=3"
           "nrf70.binslot_index_secondary=4"
      )
    endif()

    list(APPEND bin_files "${CMAKE_BINARY_DIR}/nrf70.signed.bin")
    list(APPEND zip_names "nrf70.bin")
    list(APPEND signed_targets nrf70_wifi_fw_patch_target)
  endif()

  if(bin_files)
    sysbuild_get(mcuboot_fw_info_firmware_version IMAGE mcuboot VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)

    include(${ZEPHYR_NRF_MODULE_DIR}/cmake/fw_zip.cmake)

    generate_dfu_zip(
      OUTPUT ${PROJECT_BINARY_DIR}/dfu_application.zip
      BIN_FILES ${bin_files}
      ZIP_NAMES ${zip_names}
      TYPE application
      IMAGE ${DEFAULT_IMAGE}
      SCRIPT_PARAMS ${generate_script_app_params}
      DEPENDS ${signed_targets}
      ${exclude_files}
      ${include_files}
      )
  endif()
endfunction()

dfu_app_zip_package()
