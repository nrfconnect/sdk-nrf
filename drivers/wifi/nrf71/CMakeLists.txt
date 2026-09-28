#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(nrf71_wifi)

set(nrf71_inc ${CMAKE_CURRENT_SOURCE_DIR}/inc)

zephyr_library_include_directories(
  ${nrf71_inc}
  ${ZEPHYR_BASE}/subsys/net/ip
)

# ---- Common sources ----
zephyr_library_sources(
  src/common/fmac_cmd_common.c
  src/common/fmac_api_common.c
  src/common/rf_params.c
  src/common/util.c
  src/common/mem_mgmt.c
  src/common/wifi_ipc.c
  src/common/mac_addr.c
)

if(CONFIG_NRF71_RADIO_TEST)
  zephyr_library_sources(
    src/radio_test/fmac_api.c
    src/radio_test/fmac_cmd.c
    src/radio_test/fmac_event.c
    src/radio_test/main.c
    src/radio_test/api.c
  )

  zephyr_library_include_directories(
    ${ZEPHYR_NRF_MODULE_DIR}/include/drivers/wifi/nrf71
  )

  zephyr_library_sources_ifdef(CONFIG_NRF71_RADIO_TEST_SHELL
    src/radio_test/shell.c
  )
endif()

if(CONFIG_NRF71_OFFLOADED_RAW_TX)
  zephyr_library_sources(
    src/offload_raw_tx/fmac_api.c
    src/offload_raw_tx/fmac_cmd.c
    src/offload_raw_tx/fmac_event.c
    src/offload_raw_tx/api.c
  )
endif()

if(CONFIG_NRF71_SYSTEM_MODE OR CONFIG_NRF71_SCAN_ONLY)
  zephyr_library_sources(
    src/common/nbuf_mgmt.c
    src/system/rx.c
    src/system/fmac_vif.c
    src/system/fmac_api.c
    src/system/fmac_cmd.c
    src/system/fmac_event.c
    src/system/main.c
    src/system/net_if.c
  )

  if(CONFIG_NRF71_DATA_TX)
    zephyr_library_sources(
      src/system/tx.c
      src/system/fmac_peer.c
    )
  elseif(CONFIG_NRF71_STA_MODE)
    zephyr_library_sources(src/system/fmac_peer.c)
  endif()

  if(CONFIG_NRF71_PROMISC_DATA_RX)
    zephyr_library_sources(src/system/fmac_promisc.c)
  endif()

  if(CONFIG_NRF71_AP_MODE)
    zephyr_library_sources(src/system/fmac_ap.c)
  endif()
endif()

# ---- Driver shim (Zephyr net_if, mgmt, wpa, etc.) ----
zephyr_library_sources_ifdef(CONFIG_NRF71_SR_COEX_DRIVER
  src/system/coex_transport.c
)


zephyr_library_sources_ifdef(CONFIG_NET_L2_WIFI_MGMT
  src/system/wifi_mgmt_scan.c
)

if(CONFIG_NRF71_SYSTEM_MODE OR CONFIG_NRF71_STA_MODE)
  zephyr_library_sources(src/system/wifi_mgmt.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NRF71_STA_MODE
  src/system/wpa_supp_if.c
)

if(CONFIG_NRF71_UTIL OR CONFIG_NRF71_DEBUG_SHELL)
  zephyr_library_sources(src/common/shell.c)
endif()

zephyr_library_sources_ifdef(CONFIG_NRF71_UTIL
  src/common/util_shell.c
)

zephyr_library_sources_ifdef(CONFIG_NRF71_DEBUG_SHELL
  src/common/debug_shell.c
)
