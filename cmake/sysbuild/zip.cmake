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
    set(app_update_name "${DEFAULT_IMAGE}.bin")
    set(secondary_app_update_name mcuboot_secondary_app.bin)

    if(NOT SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
      # Application
      set(generate_script_app_params
          "${app_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
          "${app_update_name}image_index=0"
          "${app_update_name}slot_index_primary=1"
          "${app_update_name}slot_index_secondary=2"
          "${app_update_name}version_MCUBOOT=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
         )
      list(APPEND bin_files "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin")
      list(APPEND zip_names ${app_update_name})
      list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts)
      set(exclude_files EXCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin)
      set(include_files INCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin)
    else()
      # Application in DirectXIP mode
      set(generate_script_app_params
          "${app_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_APP_ADDRESS>"
          "${app_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
          "${app_update_name}image_index=0"
          "${app_update_name}slot=0"
          "${secondary_app_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_SECONDARY_APP_ADDRESS>"
          "${secondary_app_update_name}image_index=0"
          "${secondary_app_update_name}slot=1"
          "${secondary_app_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
         )

      list(APPEND bin_files
           "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin"
           "${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin"
      )
      list(APPEND zip_names "${app_update_name};${secondary_app_update_name}")
      list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts mcuboot_secondary_app_extra_byproducts)
      set(exclude_files EXCLUDE
          ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin
          ${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin
      )
      set(include_files INCLUDE
          ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
          ${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
      )
    endif()
  endif()

  if(SB_CONFIG_DFU_ZIP_NET)
    # Network core
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_CPUNET)
    set(net_update_name "${image_name}.bin")
    sysbuild_get(net_core_board IMAGE ${image_name} VAR BOARD CACHE)
    sysbuild_get(net_update_version IMAGE ${image_name} VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)
    set(generate_script_app_params
        ${generate_script_app_params}
        "${net_update_name}image_index=1"
        "${net_update_name}slot_index_primary=3"
        "${net_update_name}slot_index_secondary=4"
        "${net_update_name}load_address=$<TARGET_PROPERTY:partition_manager,CPUNET_PM_APP_ADDRESS>"
        "${net_update_name}version=${net_update_version}"
        "${net_update_name}board=${net_core_board}"
        "${net_update_name}soc=${SB_CONFIG_SOC}"
       )
    list(APPEND bin_files "${CMAKE_BINARY_DIR}/signed_by_mcuboot_and_b0_${image_name}.bin")
    list(APPEND zip_names "${net_update_name}")
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
      OUTPUT ${CMAKE_BINARY_DIR}/dfu_application.zip
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
