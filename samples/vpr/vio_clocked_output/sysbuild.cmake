# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# for the nRF9251 different overlay and configuration are needed depending on which VPR used.
if("${BOARD}" MATCHES "nrf9251")
  if("${BOARD_QUALIFIERS}" MATCHES "cpuflpr")
    set(vpr_launcher_EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/sysbuild/vpr_launcher/nrf9251_flpr.overlay CACHE INTERNAL "")
  elseif("${BOARD_QUALIFIERS}" MATCHES "cpuppr")
    set(vpr_launcher_EXTRA_DTC_OVERLAY_FILE ${CMAKE_CURRENT_LIST_DIR}/sysbuild/vpr_launcher/nrf9251_ppr.overlay CACHE INTERNAL "")
    set(vpr_launcher_EXTRA_CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/sysbuild/vpr_launcher/nrf9251_ppr.conf CACHE INTERNAL "")
  endif()
endif()
