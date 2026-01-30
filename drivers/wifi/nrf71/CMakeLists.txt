#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

function(nrf71_wifi_setup_include_and_sources)
  zephyr_library_named(nrf71_wifi)

  set(nrf71_osal_base ${CMAKE_CURRENT_SOURCE_DIR}/osal)
  set(nrf71_utils_base ${CMAKE_CURRENT_SOURCE_DIR})
  set(nrf71_os_shim_dir ${CMAKE_CURRENT_SOURCE_DIR}/os)
  set(nrf71_bus_dir ${CMAKE_CURRENT_SOURCE_DIR}/bus)
  set(os_agnostic_base ${ZEPHYR_NRF_WIFI_MODULE_DIR})
  set(fw_bins_base ${ZEPHYR_NRF_WIFI_MODULE_DIR}/zephyr/blobs/wifi_fw_bins)

  zephyr_library_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../bus
    ${CMAKE_CURRENT_SOURCE_DIR}/fw_if
    ${nrf71_utils_base}/utils/inc
    ${nrf71_osal_base}/os_if/inc
    ${nrf71_osal_base}/bus_if/bus/qspi/inc
    ${nrf71_osal_base}/bus_if/bal/inc
    ${nrf71_osal_base}/fw_if/umac_if/inc
    ${nrf71_osal_base}/hw_if/hal/inc
    ${nrf71_os_shim_dir}
    ${nrf71_bus_dir}
    ${ZEPHYR_BASE}/subsys/net/ip
  )

  # ---- Sources: osal (os_if, utils, bus_if, fw_if, hw_if) ----
  zephyr_library_sources(
    ${nrf71_osal_base}/os_if/src/osal.c
    ${nrf71_utils_base}/utils/src/list.c
    ${nrf71_utils_base}/utils/src/queue.c
    ${nrf71_utils_base}/utils/src/util.c
    ${nrf71_osal_base}/hw_if/hal/src/common/hal_api_common.c
    ${nrf71_osal_base}/bus_if/bal/src/bal.c
    ${nrf71_osal_base}/bus_if/bus/qspi/src/qspi.c
    ${nrf71_osal_base}/fw_if/umac_if/src/common/fmac_cmd_common.c
    ${nrf71_osal_base}/fw_if/umac_if/src/common/fmac_api_common.c
    ${nrf71_osal_base}/fw_if/umac_if/src/common/fmac_util.c
    ${nrf71_os_shim_dir}/shim.c
    ${nrf71_os_shim_dir}/timer.c
    ${nrf71_os_shim_dir}/work.c
    ${nrf71_bus_dir}/device.c
    ${nrf71_bus_dir}/ipc_if.c
    ${nrf71_bus_dir}/ipc_service.c
    ${nrf71_bus_dir}/spsc_qm.c
  )

  if(CONFIG_NRF71_RADIO_TEST)
    zephyr_library_sources(
      ${nrf71_osal_base}/fw_if/umac_if/src/radio_test/fmac_api.c
      ${nrf71_osal_base}/fw_if/umac_if/src/radio_test/fmac_cmd.c
      ${nrf71_osal_base}/fw_if/umac_if/src/radio_test/fmac_event.c
      ${nrf71_osal_base}/hw_if/hal/src/radio_test/hal_api.c
    )
  endif()

  if(CONFIG_NRF71_OFFLOADED_RAW_TX)
    zephyr_library_sources(
      ${nrf71_osal_base}/fw_if/umac_if/src/offload_raw_tx/fmac_api.c
      ${nrf71_osal_base}/fw_if/umac_if/src/offload_raw_tx/fmac_cmd.c
      ${nrf71_osal_base}/fw_if/umac_if/src/offload_raw_tx/fmac_event.c
      ${nrf71_osal_base}/hw_if/hal/src/offload_raw_tx/hal_api.c
    )
  endif()

  if(CONFIG_NRF71_SYSTEM_MODE OR CONFIG_NRF71_SCAN_ONLY)
    zephyr_library_sources(
      ${nrf71_osal_base}/fw_if/umac_if/src/system/rx.c
      ${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_vif.c
      ${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_api.c
      ${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_cmd.c
      ${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_event.c
      ${nrf71_osal_base}/hw_if/hal/src/system/hal_api.c
    )
    if(CONFIG_NRF71_DATA_TX)
      zephyr_library_sources(
        ${nrf71_osal_base}/fw_if/umac_if/src/system/tx.c
        ${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_peer.c
      )
    endif()
    if(CONFIG_NRF71_STA_MODE)
      zephyr_library_sources(${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_peer.c)
    endif()
    if(CONFIG_NRF71_PROMISC_DATA_RX)
      zephyr_library_sources(
        ${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_promisc.c
      )
    endif()
    if(CONFIG_NRF71_AP_MODE)
      zephyr_library_sources(
        ${nrf71_osal_base}/fw_if/umac_if/src/system/fmac_ap.c
      )
    endif()
  endif()

  # Kconfig -> compile definitions
  zephyr_library_compile_definitions(
    $<$<BOOL:${CONFIG_NRF_WIFI_LOW_POWER}>:NRF_WIFI_LOW_POWER>
    $<$<BOOL:${CONFIG_NRF_WIFI_RPU_RECOVERY}>:NRF_WIFI_RPU_RECOVERY>
    $<$<BOOL:${CONFIG_NRF_WIFI_RPU_RECOVERY_DEBUG}>:NRF_WIFI_RPU_RECOVERY_DEBUG>
    $<$<BOOL:${CONFIG_NRF_WIFI_RPU_RECOVERY_PS_STATE_DEBUG}>:NRF_WIFI_RPU_RECOVERY_PS_STATE_DEBUG>
    $<$<BOOL:${CONFIG_NRF_WIFI_AP_DEAD_DETECT_TIMEOUT}>:NRF_WIFI_AP_DEAD_DETECT_TIMEOUT=${CONFIG_NRF_WIFI_AP_DEAD_DETECT_TIMEOUT}>
    $<$<BOOL:${CONFIG_NRF_WIFI_IFACE_MTU}>:NRF_WIFI_IFACE_MTU=${CONFIG_NRF_WIFI_IFACE_MTU}>
    $<$<BOOL:${CONFIG_NRF71_STA_MODE}>:NRF71_STA_MODE>
    $<$<BOOL:${CONFIG_NRF71_P2P_MODE}>:NRF71_P2P_MODE>
    $<$<BOOL:${CONFIG_NRF71_DATA_TX}>:NRF71_DATA_TX>
    $<$<BOOL:${CONFIG_NRF71_RAW_DATA_TX}>:NRF71_RAW_DATA_TX>
    $<$<BOOL:${CONFIG_NRF71_RAW_DATA_RX}>:NRF71_RAW_DATA_RX>
    $<$<BOOL:${CONFIG_NRF71_PROMISC_DATA_RX}>:NRF71_PROMISC_DATA_RX>
    $<$<BOOL:${CONFIG_NRF71_TX_DONE_WQ_ENABLED}>:NRF71_TX_DONE_WQ_ENABLED>
    $<$<BOOL:${CONFIG_NRF71_RX_WQ_ENABLED}>:NRF71_RX_WQ_ENABLED>
    $<$<BOOL:${CONFIG_NRF71_UTIL}>:NRF71_UTIL>
    $<$<BOOL:${CONFIG_NRF71_RADIO_TEST}>:NRF71_RADIO_TEST>
    $<$<BOOL:${CONFIG_NRF71_OFFLOADED_RAW_TX}>:NRF71_OFFLOADED_RAW_TX>
    $<$<BOOL:${CONFIG_NRF71_TCP_IP_CHECKSUM_OFFLOAD}>:NRF71_TCP_IP_CHECKSUM_OFFLOAD>
    $<$<BOOL:${CONFIG_NRF71_RPU_EXTEND_TWT_SP}>:NRF71_RPU_EXTEND_TWT_SP>
    $<$<BOOL:${CONFIG_NRF71_SYSTEM_WITH_RAW_MODES}>:NRF71_SYSTEM_WITH_RAW_MODES>
    $<$<BOOL:${CONFIG_NRF71_SCAN_ONLY}>:NRF71_SCAN_ONLY>
    $<$<BOOL:${CONFIG_NRF71_SYSTEM_MODE}>:NRF71_SYSTEM_MODE>
    $<$<BOOL:${CONFIG_NRF71_LOG_VERBOSE}>:NRF71_LOG_VERBOSE>
    $<$<BOOL:${CONFIG_NRF71_AP_MODE}>:NRF71_AP_MODE>
    $<$<BOOL:${CONFIG_NRF_WIFI_MGMT_BUFF_OFFLOAD}>:NRF_WIFI_MGMT_BUFF_OFFLOAD>
    $<$<BOOL:${CONFIG_NRF_WIFI_FEAT_KEEPALIVE}>:NRF_WIFI_FEAT_KEEPALIVE>
    $<$<BOOL:${CONFIG_NRF_WIFI_FEAT_KEEPALIVE}>:NRF_WIFI_KEEPALIVE_PERIOD_S=${CONFIG_NRF_WIFI_KEEPALIVE_PERIOD_S}>
    $<$<BOOL:${CONFIG_NRF_WIFI_QOS_NOACK_POLICY}>:NRF_WIFI_QOS_NOACK_POLICY>
    $<$<BOOL:${CONFIG_NRF_WIFI_QOS_NOACK_POLICY}>:NRF_WIFI_QOS_NOACK_POLICY_TID=${CONFIG_NRF_WIFI_QOS_NOACK_POLICY_TID}>
    $<$<BOOL:${CONFIG_WIFI_MGMT_RAW_SCAN_RESULTS}>:WIFI_MGMT_RAW_SCAN_RESULTS>
    $<$<BOOL:${CONFIG_NRF_WIFI_COEX_DISABLE_PRIORITY_WINDOW_FOR_SCAN}>:NRF_WIFI_COEX_DISABLE_PRIORITY_WINDOW_FOR_SCAN>
    $<$<BOOL:${CONFIG_NRF_WIFI_RX_STBC_HT}>:NRF_WIFI_RX_STBC_HT>
    $<$<BOOL:${CONFIG_NRF71_SR_COEX_SLEEP_CTRL_GPIO_CTRL}>:NRF71_SR_COEX_SLEEP_CTRL_GPIO_CTRL>
    $<$<BOOL:${CONFIG_NRF_WIFI_DYNAMIC_BANDWIDTH_SIGNALLING}>:NRF_WIFI_DYNAMIC_BANDWIDTH_SIGNALLING>
    $<$<BOOL:${CONFIG_NRF_WIFI_DYNAMIC_ED}>:NRF_WIFI_DYNAMIC_ED>
    NRF_WIFI_MAX_PS_POLL_FAIL_CNT=${CONFIG_NRF_WIFI_MAX_PS_POLL_FAIL_CNT}
    NRF71_RX_NUM_BUFS=${CONFIG_NRF71_RX_NUM_BUFS}
    NRF71_MAX_TX_TOKENS=${CONFIG_NRF71_MAX_TX_TOKENS}
    NRF71_RX_MAX_DATA_SIZE=${CONFIG_NRF71_RX_MAX_DATA_SIZE}
    NRF71_MAX_TX_PENDING_QLEN=${CONFIG_NRF71_MAX_TX_PENDING_QLEN}
    NRF71_RPU_PS_IDLE_TIMEOUT_MS=${CONFIG_NRF71_RPU_PS_IDLE_TIMEOUT_MS}
    NRF71_BAND_2G_LOWER_EDGE_BACKOFF_DSSS=${CONFIG_NRF71_BAND_2G_LOWER_EDGE_BACKOFF_DSSS}
    NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HT}
    NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_2G_LOWER_EDGE_BACKOFF_HE}
    NRF71_BAND_2G_UPPER_EDGE_BACKOFF_DSSS=${CONFIG_NRF71_BAND_2G_UPPER_EDGE_BACKOFF_DSSS}
    NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HT}
    NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_2G_UPPER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE}
    NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT=${CONFIG_NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT}
    NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE=${CONFIG_NRF71_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE}
    NRF71_PCB_LOSS_2G=${CONFIG_NRF71_PCB_LOSS_2G}
    NRF71_PCB_LOSS_5G_BAND1=${CONFIG_NRF71_PCB_LOSS_5G_BAND1}
    NRF71_PCB_LOSS_5G_BAND2=${CONFIG_NRF71_PCB_LOSS_5G_BAND2}
    NRF71_PCB_LOSS_5G_BAND3=${CONFIG_NRF71_PCB_LOSS_5G_BAND3}
    NRF71_ANT_GAIN_2G=${CONFIG_NRF71_ANT_GAIN_2G}
    NRF71_ANT_GAIN_5G_BAND1=${CONFIG_NRF71_ANT_GAIN_5G_BAND1}
    NRF71_ANT_GAIN_5G_BAND2=${CONFIG_NRF71_ANT_GAIN_5G_BAND2}
    NRF71_ANT_GAIN_5G_BAND3=${CONFIG_NRF71_ANT_GAIN_5G_BAND3}
    NRF_WIFI_PS_INT_PS=${CONFIG_NRF_WIFI_PS_INT_PS}
    NRF_WIFI_RPU_RECOVERY_PS_ACTIVE_TIMEOUT_MS=${CONFIG_NRF_WIFI_RPU_RECOVERY_PS_ACTIVE_TIMEOUT_MS}
    NRF_WIFI_DISPLAY_SCAN_BSS_LIMIT=${CONFIG_NRF_WIFI_DISPLAY_SCAN_BSS_LIMIT}
    NRF_WIFI_RPU_MIN_TIME_TO_ENTER_SLEEP_MS=${CONFIG_NRF_WIFI_RPU_MIN_TIME_TO_ENTER_SLEEP_MS}
    $<$<BOOL:${CONFIG_NRF_WIFI_CMD_EVENT_LOG}>:NRF_WIFI_CMD_EVENT_LOG>
  )

  if(DEFINED CONFIG_WIFI_NRF71_LOG_LEVEL AND NOT "${CONFIG_WIFI_NRF71_LOG_LEVEL}" STREQUAL "")
    zephyr_library_compile_definitions(WIFI_NRF71_LOG_LEVEL=${CONFIG_WIFI_NRF71_LOG_LEVEL})
  endif()

  zephyr_library_compile_definitions(NRF_WIFI_RX_BUFF_PROG_UMAC)

  zephyr_library_include_directories_ifdef(CONFIG_NRF71_OFFLOADED_RAW_TX
    off_raw_tx/inc
  )


  # ---- Sources: driver (Zephyr shim, net_if, mgmt, wpa, etc.) ----
  zephyr_library_sources_ifdef(CONFIG_NRF71_SR_COEX
    src/coex.c
  )

  zephyr_library_sources_ifndef(CONFIG_NRF71_OFFLOADED_RAW_TX
    src/fmac_main.c
  )

  if(NOT CONFIG_NRF71_RADIO_TEST AND NOT CONFIG_NRF71_OFFLOADED_RAW_TX)
    zephyr_library_sources(src/net_if.c)
  endif()

  zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_MGMT
    src/wifi_mgmt_scan.c
  )

  zephyr_library_sources_ifdef(CONFIG_NRF71_SYSTEM_MODE
    src/wifi_mgmt.c
  )

  zephyr_library_sources_ifdef(CONFIG_NRF71_OFFLOADED_RAW_TX
    off_raw_tx/src/off_raw_tx_api.c
  )

  zephyr_library_sources_ifdef(CONFIG_NRF71_STA_MODE
    src/wpa_supp_if.c
    src/wifi_mgmt.c
  )

  zephyr_library_sources_ifdef(CONFIG_NRF71_UTIL
    src/wifi_util.c
  )

  if(CONFIG_NRF71_UTIL OR CONFIG_NRF71_DEBUG_SHELL)
    zephyr_library_sources(src/shell.c)
  endif()

  zephyr_library_sources_ifdef(CONFIG_NRF71_DEBUG_SHELL
    src/debug_shell.c
  )
endfunction()

nrf71_wifi_setup_include_and_sources()
