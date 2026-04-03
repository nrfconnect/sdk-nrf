#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_NRF_RADIO_LOADER)
  add_overlay_dts(
    ${DEFAULT_IMAGE}
    ${CMAKE_CURRENT_LIST_DIR}/sysbuild/tcm_cpuapp.overlay
  )

  if(DEFINED SB_CONFIG_NETCORE_IMAGE_NAME AND NOT (SB_CONFIG_NETCORE_IMAGE_NAME STREQUAL ""))
    add_overlay_dts(
      ${SB_CONFIG_NETCORE_IMAGE_NAME}
      ${CMAKE_CURRENT_LIST_DIR}/sysbuild/ipc_radio/tcm_nrf54h20.overlay
    )
    add_overlay_config(
      ${SB_CONFIG_NETCORE_IMAGE_NAME}
      ${CMAKE_CURRENT_LIST_DIR}/sysbuild/ipc_radio/tcm_nrf54h20.conf
    )
  endif()
endif()
