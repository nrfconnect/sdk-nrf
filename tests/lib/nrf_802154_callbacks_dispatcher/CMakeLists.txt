#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_802154_callbacks_dispatcher_test)

set(dispatcher_lib_dir "${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_802154_callbacks_dispatcher")
set(dispatcher_source "${dispatcher_lib_dir}/nrf_802154_callbacks_dispatcher.c")
set(dispatcher_test_api_header "${CMAKE_CURRENT_SOURCE_DIR}/stubs/nrf_802154_test_api.h")

target_sources(app PRIVATE
  src/main.c
  stubs/nrf_802154_stubs.c
  ${dispatcher_source}
)

zephyr_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_802154/common/include
)

zephyr_linker_sources(
  SECTIONS
  ${dispatcher_lib_dir}/callbacks.ld
)

target_compile_definitions(app PRIVATE
  CONFIG_NRF_802154_CALLBACKS_DISPATCHER=1
  CONFIG_NRF_802154_CALLBACKS_DISPATCHER_INIT_PRIORITY=80
  NRF_802154_DRV_REINIT_ENABLED=1
  NRF_802154_SERIALIZATION_HOST=1
)

# nrf_802154.h omits host-side driver APIs when NRF_802154_SERIALIZATION_HOST is set.
set_source_files_properties(${dispatcher_source}
  PROPERTIES
  COMPILE_OPTIONS "-include;${dispatcher_test_api_header}"
)
