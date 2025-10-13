# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

function(mcuboot_image_number_to_slot result image secondary)
  if(secondary)
    set(secondary_offset "+ 1")
  else()
    set(secondary_offset "")
  endif()

  math(EXPR slot "${image} * 2 ${secondary_offset}")

  set(${result} ${slot} PARENT_SCOPE)
endfunction()

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
    if(SB_CONFIG_QSPI_XIP_SPLIT_IMAGE)
      set(app_internal_update_name "${DEFAULT_IMAGE}.internal.bin")
      set(app_external_update_name "${DEFAULT_IMAGE}.external.bin")
      set(secondary_app_internal_update_name mcuboot_secondary_app.internal.bin)
      set(secondary_app_external_update_name mcuboot_secondary_app.external.bin)
      mcuboot_image_number_to_slot(internal_slot_primary ${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER} n)
      mcuboot_image_number_to_slot(internal_slot_secondary ${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER} y)
      mcuboot_image_number_to_slot(external_slot_primary ${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER} n)
      mcuboot_image_number_to_slot(external_slot_secondary ${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER} y)

      if(NOT SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
        # Application
        math(EXPR internal_slot_primary "${internal_slot_primary} + 1")
        math(EXPR internal_slot_secondary "${internal_slot_secondary} + 1")
        math(EXPR external_slot_primary "${external_slot_primary} + 1")
        math(EXPR external_slot_secondary "${external_slot_secondary} + 1")

        set(generate_script_app_params
            "${app_internal_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>"
            "${app_internal_update_name}image_index=${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER}"
            "${app_internal_update_name}slot_index_primary=${internal_slot_primary}"
            "${app_internal_update_name}slot_index_secondary=${internal_slot_secondary}"
            "${app_internal_update_name}version_MCUBOOT=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
            "${app_external_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_APP_ADDRESS>"
            "${app_external_update_name}image_index=${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}"
            "${app_external_update_name}slot_index_primary=${external_slot_primary}"
            "${app_external_update_name}slot_index_secondary=${external_slot_secondary}"
            "${app_external_update_name}version_MCUBOOT=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
           )
        list(APPEND bin_files "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.internal.signed.bin")
        list(APPEND bin_files "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.external.signed.bin")
        list(APPEND zip_names "${app_internal_update_name};${app_external_update_name}")
        list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts)
        set(exclude_files EXCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.internal.signed.bin)
        set(exclude_files EXCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.external.signed.bin)
        set(include_files INCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin)
      else()
        # Application in DirectXIP mode
        set(generate_script_app_params
            "${app_internal_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_APP_ADDRESS>"
            "${app_internal_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
            "${app_internal_update_name}image_index=${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER}"
            "${app_internal_update_name}slot=${internal_slot_primary}"
            "${app_external_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_APP_ADDRESS>"
            "${app_external_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
            "${app_external_update_name}image_index=${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}"
            "${app_external_update_name}slot=${external_slot_primary}"
            "${secondary_app_internal_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_SECONDARY_APP_ADDRESS>"
            "${secondary_app_internal_update_name}image_index=0"
            "${secondary_app_internal_update_name}slot=${internal_slot_secondary}"
            "${secondary_app_internal_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
            "${secondary_app_external_update_name}load_address=$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_SECONDARY_${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}_APP_ADDRESS>"
            "${secondary_app_external_update_name}image_index=${NCS_MCUBOOT_QSPI_XIP_IMAGE_NUMBER}"
            "${secondary_app_external_update_name}slot=${external_slot_secondary}"
            "${secondary_app_external_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
           )

        list(APPEND bin_files
             "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.internal.signed.bin"
             "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.external.signed.bin"
             "${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.internal.signed.bin"
             "${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.external.signed.bin"
        )
        list(APPEND zip_names "${app_internal_update_name};${app_external_update_name};${secondary_app_internal_update_name};${secondary_app_external_update_name}")
        list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts mcuboot_secondary_app_extra_byproducts)
        set(exclude_files EXCLUDE
            ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.internal.signed.bin
            ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.external.signed.bin
            ${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.internal.signed.bin
            ${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.external.signed.bin
        )
        set(include_files INCLUDE
            ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
            ${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
        )
      endif()
    else()
      if(SB_CONFIG_BOOT_ENCRYPTION)
        set(app_update_name "${DEFAULT_IMAGE}.signed.encrypted.bin")
      else()
        set(app_update_name "${DEFAULT_IMAGE}.signed.bin")
        set(secondary_app_update_name mcuboot_secondary_app.signed.bin)
      endif()

      mcuboot_image_number_to_slot(slot_primary ${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER} n)
      mcuboot_image_number_to_slot(slot_secondary ${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER} y)

      if(NOT SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
        if(SB_CONFIG_PARTITION_MANAGER)
          set(load_address "$<TARGET_PROPERTY:partition_manager,PM_APP_ADDRESS>")
        else()
          get_address_from_dt_partition_nodelabel("slot${slot_primary}_partition" load_address)
        endif()

        # Application
        math(EXPR slot_primary "${slot_primary} + 1")
        math(EXPR slot_secondary "${slot_secondary} + 1")

        set(generate_script_app_params
            "${app_update_name}load_address=${load_address}"
            "${app_update_name}image_index=${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER}"
            "${app_update_name}slot_index_primary=${slot_primary}"
            "${app_update_name}slot_index_secondary=${slot_secondary}"
            "${app_update_name}version_MCUBOOT=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
           )
        if(SB_CONFIG_BOOT_ENCRYPTION)
          list(APPEND bin_files "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.encrypted.bin")
          set(exclude_files EXCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.encrypted.bin)
        else()
          list(APPEND bin_files "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin")
          set(exclude_files EXCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin)
        endif()
        list(APPEND zip_names ${app_update_name})
        list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts)
        set(include_files INCLUDE ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin)
      else()
        if(SB_CONFIG_PARTITION_MANAGER)
          set(primary_load_address "$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_PRIMARY_APP_ADDRESS>")
          set(secondary_load_address "$<TARGET_PROPERTY:partition_manager,PM_MCUBOOT_SECONDARY_APP_ADDRESS>")
        else()
          get_address_from_dt_partition_nodelabel("slot${slot_primary}_partition" primary_load_address)
          get_address_from_dt_partition_nodelabel("slot${slot_secondary}_partition" secondary_load_address)
        endif()

        # Application in DirectXIP mode
        set(generate_script_app_params
            "${app_update_name}load_address=${primary_load_address}"
            "${app_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
            "${app_update_name}image_index=${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER}"
            "${app_update_name}slot=${slot_primary}"
            "${secondary_app_update_name}load_address=${secondary_load_address}"
            "${secondary_app_update_name}image_index=${NCS_MCUBOOT_APPLICATION_IMAGE_NUMBER}"
            "${secondary_app_update_name}slot=${slot_secondary}"
            "${secondary_app_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
           )

        if(NOT SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
          list(APPEND bin_files
               "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin"
               "${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin"
          )
          set(exclude_files EXCLUDE
              ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin
              ${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin
          )
          set(include_files INCLUDE
              ${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
              ${CMAKE_BINARY_DIR}/mcuboot_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
          )
        else()
          list(APPEND bin_files
               "${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin"
               "${CMAKE_BINARY_DIR}/zephyr/${CONFIG_KERNEL_BIN_NAME}_secondary_app.signed.bin"
          )
        endif()

        list(APPEND zip_names "${app_update_name};${secondary_app_update_name}")
        list(APPEND signed_targets ${DEFAULT_IMAGE}_extra_byproducts mcuboot_secondary_app_extra_byproducts)
      endif()
    endif()
  endif()

  if(SB_CONFIG_DFU_ZIP_NET AND NOT SB_CONFIG_MCUBOOT_SIGN_MERGED_BINARY)
    # Network core
    get_property(image_name GLOBAL PROPERTY DOMAIN_APP_CPUNET)
    set(net_update_name "${image_name}.bin")
    set(secondary_net_update_name "${image_name}_secondary_app.bin")
    sysbuild_get(net_core_board IMAGE ${image_name} VAR BOARD CACHE)

    if(SB_CONFIG_SECURE_BOOT_NETCORE)
      sysbuild_get(net_update_version IMAGE ${image_name} VAR CONFIG_FW_INFO_FIRMWARE_VERSION KCONFIG)
    else()
      sysbuild_get(net_update_version IMAGE ${image_name} VAR CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION KCONFIG)
    endif()

    mcuboot_image_number_to_slot(net_update_slot_primary ${NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER} n)
    mcuboot_image_number_to_slot(net_update_slot_secondary ${NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER} y)

    if(NOT SB_CONFIG_MCUBOOT_BUILD_DIRECT_XIP_VARIANT)
      if(SB_CONFIG_PARTITION_MANAGER)
        set(net_load_address "$<TARGET_PROPERTY:partition_manager,CPUNET_PM_APP_ADDRESS>")
      else()
        get_address_from_dt_partition_nodelabel("slot${net_update_slot_primary}_partition"
                                                net_load_address
                                               )
      endif()

      math(EXPR net_update_slot_primary "${net_update_slot_primary} + 1")
      math(EXPR net_update_slot_secondary "${net_update_slot_secondary} + 1")

      set(generate_script_app_params
          ${generate_script_app_params}
          "${net_update_name}image_index=${NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER}"
          "${net_update_name}slot_index_primary=${net_update_slot_primary}"
          "${net_update_name}slot_index_secondary=${net_update_slot_secondary}"
          "${net_update_name}load_address=${net_load_address}"
          "${net_update_name}version=${net_update_version}"
          "${net_update_name}board=${net_core_board}"
          "${net_update_name}soc=${SB_CONFIG_SOC}"
         )

      if(SB_CONFIG_SECURE_BOOT_NETCORE)
        list(APPEND bin_files "${CMAKE_BINARY_DIR}/signed_by_mcuboot_and_b0_${image_name}.bin")
        list(APPEND signed_targets ${image_name}_signed_packaged_target)
      else()
          sysbuild_get(net_CONFIG_KERNEL_BIN_NAME IMAGE ${image_name} VAR CONFIG_KERNEL_BIN_NAME KCONFIG)
          if(SB_CONFIG_BOOT_ENCRYPTION)
            list(APPEND bin_files "${CMAKE_BINARY_DIR}/${image_name}/zephyr/${net_CONFIG_KERNEL_BIN_NAME}.signed.encrypted.bin")
          else()
            list(APPEND bin_files "${CMAKE_BINARY_DIR}/${image_name}/zephyr/${net_CONFIG_KERNEL_BIN_NAME}.signed.bin")
          endif()
      endif()

      list(APPEND zip_names "${net_update_name}")
      list(APPEND signed_targets ${image_name}_extra_byproducts)
    elseif(NOT SB_CONFIG_PARTITION_MANAGER)
      get_address_from_dt_partition_nodelabel("slot${net_update_slot_primary}_partition" primary_net_load_address)
      get_address_from_dt_partition_nodelabel("slot${net_update_slot_secondary}_partition" secondary_net_load_address)

      # Radio in DirectXIP mode
      set(generate_script_app_params
        ${generate_script_app_params}
          "${net_update_name}load_address=${primary_net_load_address}"
          "${net_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
          "${net_update_name}image_index=${NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER}"
          "${net_update_name}slot=${net_update_slot_primary}"
          "${secondary_net_update_name}load_address=${secondary_net_load_address}"
          "${secondary_net_update_name}image_index=${NCS_MCUBOOT_NETWORK_CORE_IMAGE_NUMBER}"
          "${secondary_net_update_name}slot=${net_update_slot_secondary}"
          "${secondary_net_update_name}version_MCUBOOT+XIP=${CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION}"
        )
      list(APPEND bin_files
           "${CMAKE_BINARY_DIR}/${image_name}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin"
           "${CMAKE_BINARY_DIR}/${image_name}_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin"
      )
      set(exclude_files EXCLUDE
          ${CMAKE_BINARY_DIR}/${image_name}/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin
          ${CMAKE_BINARY_DIR}/${image_name}_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.signed.bin
      )
      set(include_files INCLUDE
          ${CMAKE_BINARY_DIR}/${image_name}/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
          ${CMAKE_BINARY_DIR}/${image_name}_secondary_app/zephyr/${CONFIG_KERNEL_BIN_NAME}.bin
      )

      list(APPEND zip_names "${net_update_name};${secondary_net_update_name}")
      list(APPEND signed_targets ${image_name}_extra_byproducts)
      list(APPEND signed_targets ${image_name}_secondary_app_extra_byproducts)
    endif()
  endif()

  if(SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH)
    # nRF7x Wifi patch
    mcuboot_image_number_to_slot(nrf70_patches_slot_primary ${NCS_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER} n)
    mcuboot_image_number_to_slot(nrf70_patches_slot_secondary ${NCS_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER} y)
    math(EXPR nrf70_patches_slot_primary "${nrf70_patches_slot_primary} + 1")
    math(EXPR nrf70_patches_slot_secondary "${nrf70_patches_slot_secondary} + 1")

    list(APPEND generate_script_app_params
         "nrf70.binimage_index=${NCS_MCUBOOT_WIFI_PATCHES_IMAGE_NUMBER}"
         "nrf70.binslot_index_primary=${nrf70_patches_slot_primary}"
         "nrf70.binslot_index_secondary=${nrf70_patches_slot_secondary}"
    )

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
