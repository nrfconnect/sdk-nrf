#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(source_dir ${ZEPHYR_BASE}/samples/tfm_integration/tfm_secure_partition)

get_target_property(TFM_BINARY_DIR tfm TFM_BINARY_DIR)
configure_file(
  ${source_dir}/dummy_partition/tfm_manifest_list.yaml.in
  ${CMAKE_CURRENT_BINARY_DIR}/dummy_partition/tfm_manifest_list.yaml
)

if(CONFIG_TFM_IPC)
  set(SPM_BACKEND "IPC")
else()
  set(SPM_BACKEND "SFN")
endif()

configure_file(
  ${source_dir}/dummy_partition/tfm_dummy_partition.yaml.in
  ${CMAKE_CURRENT_BINARY_DIR}/dummy_partition/tfm_dummy_partition.yaml
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DTFM_EXTRA_MANIFEST_LIST_FILES=${CMAKE_CURRENT_BINARY_DIR}/dummy_partition/tfm_manifest_list.yaml
  -DTFM_EXTRA_PARTITION_PATHS=${source_dir}/dummy_partition
)

project(tfm_secure_partition)

target_sources(app PRIVATE
  ${source_dir}/src/main.c
  ${source_dir}/src/dummy_partition.c
)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
)

target_compile_definitions(app
  PRIVATE TFM_PARTITION_DUMMY_PARTITION
)
