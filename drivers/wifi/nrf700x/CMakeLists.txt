#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

set(OS_AGNOSTIC_BASE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_wifi)

zephyr_include_directories(
  inc
  ${OS_AGNOSTIC_BASE}/utils/inc
  ${OS_AGNOSTIC_BASE}/os_if/inc
  ${OS_AGNOSTIC_BASE}/bus_if/bus/qspi/inc
  ${OS_AGNOSTIC_BASE}/bus_if/bal/inc
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc
  ${OS_AGNOSTIC_BASE}/fw_load/mips/fw/inc
  ${OS_AGNOSTIC_BASE}/hw_if/hal/inc
  src/qspi/inc
  # for net_sprint_ll_addr
  ${ZEPHYR_BASE}/subsys/net/ip
  ${OS_AGNOSTIC_BASE}/hw_if/hal/inc/fw
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/fw
)

zephyr_include_directories_ifdef(CONFIG_NRF700X_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/radio_test
)

zephyr_include_directories_ifndef(CONFIG_NRF700X_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/inc/default
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_SR_COEX
  src/coex.c
)

zephyr_library_sources(
  ${OS_AGNOSTIC_BASE}/os_if/src/osal.c
  ${OS_AGNOSTIC_BASE}/utils/src/list.c
  ${OS_AGNOSTIC_BASE}/utils/src/queue.c
  ${OS_AGNOSTIC_BASE}/utils/src/util.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_api.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_fw_patch_loader.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_interrupt.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_mem.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hal_reg.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/hpqm.c
  ${OS_AGNOSTIC_BASE}/hw_if/hal/src/pal.c
  ${OS_AGNOSTIC_BASE}/bus_if/bal/src/bal.c
  ${OS_AGNOSTIC_BASE}/bus_if/bus/qspi/src/qspi.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/cmd.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/event.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_api_common.c
  src/shim.c
  src/work.c
  src/timer.c
  src/fmac_main.c
  src/fw_load.c
  src/qspi/src/device.c
  src/qspi/src/rpu_hw_if.c
  src/qspi/src/ficr_prog.c
)

zephyr_library_sources_ifndef(CONFIG_NRF700X_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/rx.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_vif.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
  src/net_if.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/default/fmac_api.c
)

zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_MGMT
  src/wifi_mgmt_scan.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_SYSTEM_MODE
  src/wifi_mgmt.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_SCAN_ONLY
  src/wifi_mgmt_scan.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_RADIO_TEST
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/radio_test/fmac_api.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_DATA_TX
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/tx.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_STA_MODE
  src/wpa_supp_if.c
  src/wifi_mgmt.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_AP_MODE
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_ap.c
)


# Without WPA supplicant we only support scan
zephyr_library_sources_ifdef(CONFIG_NRF700X_STA_MODE
  src/wpa_supp_if.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_peer.c
  ${OS_AGNOSTIC_BASE}/fw_if/umac_if/src/fmac_util.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_QSPI
  src/qspi/src/qspi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_ON_SPI
  src/qspi/src/spi_if.c
)

zephyr_library_sources_ifdef(CONFIG_NRF700X_UTIL
  src/wifi_util.c
)

if(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP)
# Run patches from the external flash (XIP). No need to copy.
zephyr_code_relocate(FILES src/fw_load.c LOCATION EXTFLASH_RODATA NOCOPY)
endif()

zephyr_compile_definitions_ifdef(CONFIG_NRF700X_ON_QSPI
# These are XIP related anomalies and aren't applicable for nRF7002 and cause
# throughput issues.
  -DNRF53_ERRATA_43_ENABLE_WORKAROUND=0
  -DNRF52_ERRATA_215_ENABLE_WORKAROUND=0
# nRF70 QSPI doesn't use 192MHz clock and most samples use 128MHz, this can cause anomaly 159
# but as its rare and not seen in most cases, we can disable it.
# Alternative is 128MHz CPU should be disabled that impacts Wi-Fi performance.
  -DNRF53_ERRATA_159_ENABLE_WORKAROUND=0
)

# RPU FW patch binaries based on the selected configuration
if(CONFIG_NRF700X_SYSTEM_MODE)
  set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/default/nrf70.bin)
elseif(CONFIG_NRF700X_RADIO_TEST)
  set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/radio_test/nrf70.bin)
elseif(CONFIG_NRF700X_SCAN_ONLY)
  set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/scan_only/nrf70.bin)
elseif (CONFIG_NRF700X_SYSTEM_WITH_RAW_MODES)
  set(NRF70_PATCH ${OS_AGNOSTIC_BASE}/fw_bins/system_with_raw/nrf70.bin)
else()
  # Error
  message(FATAL_ERROR "Unsupported nRF70 patch configuration")
endif()

zephyr_compile_definitions(
  -DCONFIG_NRF_WIFI_FW_BIN=${NRF70_PATCH}
)

if(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_STORE)
  message(STATUS "nRF WiFi FW patch binary will be stored in external flash")

  dt_chosen(ext_flash_dev PROPERTY nordic,pm-ext-flash)
  if(DEFINED ext_flash_dev)
    get_filename_component(qspi_node ${ext_flash_dev} DIRECTORY)
  else()
    dt_nodelabel(qspi_node NODELABEL "qspi")
  endif()
  if(DEFINED qspi_node)
    dt_reg_addr(xip_addr PATH ${qspi_node} NAME qspi_mm)
    if(NOT DEFINED xip_addr)
      message(FATAL_ERROR "Unable to find XIP area in dts")
    endif()
  else()
    message(FATAL_ERROR "QSPI node not found in dts")
  endif()

  if(CONFIG_PARTITION_MANAGER_ENABLED)
    if(NOT SYSBUILD)
      add_custom_target(nrf70_wifi_fw_patch_target
        DEPENDS ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
      )
      add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
        COMMAND ${Python3_EXECUTABLE}
          -c "import sys; import intelhex; intelhex.bin2hex(sys.argv[1], sys.argv[2], int(sys.argv[3])) "
          ${NRF70_PATCH}
          ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
          $<TARGET_PROPERTY:partition_manager,nrf70_wifi_fw_XIP_ABS_ADDR>
          VERBATIM
        )
      # Delegate merging WiFi FW patch to mcuboot because we need to merge signed hex instead of raw nrf70.hex.
      if(NOT CONFIG_NRF_WIFI_FW_PATCH_DFU)
        set_property(
          GLOBAL PROPERTY
          nrf70_wifi_fw_PM_HEX_FILE
          ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
          )
        set_property(
          GLOBAL PROPERTY
          nrf70_wifi_fw_PM_TARGET
          nrf70_wifi_fw_patch_target
          )
      endif()
    endif()
  else()
    dt_nodelabel(nrf70_fw_node NODELABEL "nrf70_fw_partition")
    if(NOT DEFINED nrf70_fw_node)
      message(FATAL_ERROR "nrf70_fw_partition node not found in dts")
    endif()
    dt_prop(nrf70_fw_offset PATH ${nrf70_fw_node} PROPERTY reg INDEX 0)
    if(NOT DEFINED nrf70_fw_offset)
      message(FATAL_ERROR "nrf70_fw_partition.reg not found in dts")
    endif()
    math(EXPR NRF70_PATCH_ADDR "${xip_addr} + ${nrf70_fw_offset}")

    # Using python module instead of intelhex command line tool to handle windows
    execute_process(
      COMMAND ${Python3_EXECUTABLE}
        -c "import sys; import intelhex; intelhex.bin2hex(sys.argv[1], sys.argv[2], int(sys.argv[3]))"
        ${NRF70_PATCH}
        ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex
        ${NRF70_PATCH_ADDR}
      RESULT_VARIABLE _bin2hex_result
      OUTPUT_VARIABLE _bin2hex_output
      ERROR_VARIABLE _bin2hex_error
      COMMAND_ECHO STDOUT
    )

    if(NOT _bin2hex_result EQUAL 0)
      message(FATAL_ERROR "Failed to convert binary patch to hex format: \n${_bin2hex_output}\n${_bin2hex_error}")
    endif()

    set_property(GLOBAL APPEND PROPERTY HEX_FILES_TO_MERGE
      ${CMAKE_BINARY_DIR}/zephyr/nrf70.hex ${CMAKE_BINARY_DIR}/zephyr/zephyr.hex)
    set_property(TARGET runners_yaml_props_target PROPERTY
      hex_file ${CMAKE_BINARY_DIR}/zephyr/merged.hex)
  endif()
endif()
