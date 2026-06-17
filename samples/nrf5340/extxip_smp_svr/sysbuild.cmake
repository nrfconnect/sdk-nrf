# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.20.0)

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

if(FILE_SUFFIX STREQUAL "no_network_core_directxip")
  set(extxip_memory_map ${APP_DIR}/dts/nrf5340_extxip_memory_map_directxip.dtsi)
elseif(FILE_SUFFIX STREQUAL "no_network_core")
  set(extxip_memory_map ${APP_DIR}/dts/nrf5340_extxip_memory_map_no_network_core.dtsi)
else()
  set(extxip_memory_map ${APP_DIR}/dts/nrf5340_extxip_memory_map_netcore.dtsi)
endif()

if(SB_CONFIG_BOARD_IS_NON_SECURE)
  if(FILE_SUFFIX STREQUAL "no_network_core_directxip")
    set(extxip_app_memory_map ${APP_DIR}/dts/nrf5340_extxip_memory_map_directxip_ns.dtsi)
  elseif(FILE_SUFFIX STREQUAL "no_network_core")
    set(extxip_app_memory_map ${APP_DIR}/dts/nrf5340_extxip_memory_map_no_network_core_ns.dtsi)
  else()
    set(extxip_app_memory_map ${APP_DIR}/dts/nrf5340_extxip_memory_map_netcore_ns.dtsi)
  endif()
else()
  set(extxip_app_memory_map ${extxip_memory_map})
endif()

add_overlay_dts(${DEFAULT_IMAGE} ${extxip_app_memory_map})
add_overlay_dts(mcuboot ${extxip_memory_map})
