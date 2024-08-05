#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

function(setup_nrf700x_xip_data)
  sysbuild_dt_nodelabel(qspi_nodelabel IMAGE ${DEFAULT_IMAGE} NODELABEL "qspi")
  sysbuild_dt_reg_addr(qspi_xip_address IMAGE ${DEFAULT_IMAGE} PATH "${qspi_nodelabel}" NAME "qspi_mm")

  set(OS_AGNOSTIC_BASE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_wifi)

  if(SB_CONFIG_WIFI_NRF70_SYSTEM_MODE)
    set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/default/nrf70.bin)
  elseif(SB_CONFIG_WIFI_NRF70_RADIO_TEST)
    set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/radio_test/nrf70.bin)
  elseif(SB_CONFIG_WIFI_NRF70_SCAN_ONLY)
    set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/scan_only/nrf70.bin)
  elseif(SB_CONFIG_WIFI_NRF70_SYSTEM_WITH_RAW_MODES)
    set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/system_with_raw/nrf70.bin)
  else()
    # Error
    message(FATAL_ERROR "Unsupported nRF70 patch configuration")
  endif()

  if(SB_CONFIG_PARTITION_MANAGER)
    message(STATUS "nRF WiFi FW patch binary will be stored in external flash")

    add_custom_target(nrf70_wifi_fw_patch_target
      DEPENDS ${CMAKE_BINARY_DIR}/nrf70.hex
    )

    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/nrf70.hex
      COMMAND ${Python3_EXECUTABLE}
        -c "import sys; import intelhex; intelhex.bin2hex(sys.argv[1], sys.argv[2], int(sys.argv[3], 16) + int(sys.argv[4], 16)) "
        ${NRF70_PATCH}
        ${CMAKE_BINARY_DIR}/nrf70.hex
        $<TARGET_PROPERTY:partition_manager,PM_NRF70_WIFI_FW_OFFSET>
        ${qspi_xip_address}
        VERBATIM
      )

    # Delegate merging WiFi FW patch to mcuboot because we need to merge signed hex instead of raw nrf70.hex.
    if(SB_CONFIG_DFU_MULTI_IMAGE_PACKAGE_WIFI_FW_PATCH OR SB_CONFIG_DFU_ZIP_WIFI_FW_PATCH)
      include(${CMAKE_CURRENT_LIST_DIR}/image_signing_nrf700x.cmake)
      nrf7x_signing_tasks(${CMAKE_BINARY_DIR}/nrf70.hex ${CMAKE_BINARY_DIR}/nrf70.signed.hex ${CMAKE_BINARY_DIR}/nrf70.signed.bin nrf70_wifi_fw_patch_target)

      set_property(
        GLOBAL PROPERTY
        nrf70_wifi_fw_PM_HEX_FILE
        ${CMAKE_BINARY_DIR}/nrf70.signed.hex
        )
    else()
      set_property(
        GLOBAL PROPERTY
        nrf70_wifi_fw_PM_HEX_FILE
        ${CMAKE_BINARY_DIR}/nrf70.hex
        )
    endif()

    set_property(
      GLOBAL PROPERTY
      nrf70_wifi_fw_PM_TARGET
      nrf70_wifi_fw_patch_target
      )
  endif()
endfunction()

setup_nrf700x_xip_data()
