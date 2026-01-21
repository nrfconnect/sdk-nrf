#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT CONFIG_BUILD_WITH_TFM)
  message(WARNING "Unsupported configuration!! CONFIG_BUILD_WITH_TFM is not enabled in this build"
                  " which is required. Either the secure only variant of the board is selected or"
                  " the CONFIG_BUILD_WITH_TFM was disabled through menuconfig. Make sure that you"
                  " build with the non-secure variant of your board (target ends with /ns) and"
                  " that you have CONFIG_BUILD_WITH_TFM enabled in your build. It is advised to"
                  " build with pristine build (append -p in the build) after you encounter this .")
endif()

if(CONFIG_SOC_SERIES_NRF53 OR CONFIG_SOC_SERIES_NRF91)
  set(TFM_PERIPHERAL_TIMER_N  "TFM_PERIPHERAL_TIMER1")
  set(TFM_PERIPHERAL_SPIM_N   "TFM_PERIPHERAL_SPIM3")
  set(TFM_TIMER_N_IRQ         "TFM_TIMER1_IRQ")
  set(TFM_SPIM_N_IRQ          "TFM_SPIM3_IRQ")
  set(TFM_GPIOTE_N_IRQ        "TFM_GPIOTE0_IRQ")
  set(GPIOTE_MMIO             "")
elseif(CONFIG_SOC_SERIES_NRF54L)
  set(TFM_PERIPHERAL_TIMER_N  "TFM_PERIPHERAL_TIMER10")
  set(TFM_PERIPHERAL_SPIM_N   "TFM_PERIPHERAL_SPIM21")
  set(TFM_TIMER_N_IRQ         "TFM_TIMER10_IRQ")
  set(TFM_SPIM_N_IRQ          "TFM_SPIM21_IRQ")
  set(TFM_GPIOTE_N_IRQ        "TFM_GPIOTE20_1_IRQ")
  set(GPIOTE_MMIO [[
    {
      "name": "TFM_PERIPHERAL_GPIOTE20",
      "permission": "READ-WRITE"
    },
    ]])
else()
  message(FATAL_ERROR "Board not supported.")
endif()

get_target_property(TFM_BINARY_DIR tfm TFM_BINARY_DIR)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/secure_peripheral_partition/tfm_secure_peripheral_partition.yaml.in
  ${CMAKE_CURRENT_BINARY_DIR}/secure_peripheral_partition/tfm_secure_peripheral_partition.yaml
)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/secure_peripheral_partition/tfm_manifest_list.yaml.in
  ${CMAKE_CURRENT_BINARY_DIR}/secure_peripheral_partition/tfm_manifest_list.yaml
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DTFM_EXTRA_MANIFEST_LIST_FILES=${CMAKE_CURRENT_BINARY_DIR}/secure_peripheral_partition/tfm_manifest_list.yaml
  -DTFM_EXTRA_PARTITION_PATHS=${CMAKE_CURRENT_LIST_DIR}/secure_peripheral_partition
)

project(tfm_secure_partition)

target_sources(app PRIVATE
  src/main.c
  src/secure_peripheral_partition.c
)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
)

target_compile_definitions(app
    PRIVATE TFM_PARTITION_SECURE_PERIPHERAL_PARTITION
)
