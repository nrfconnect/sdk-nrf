#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test_suit_storage_nrf54h20)
include(../cmake/test_template.cmake)

target_include_directories(app PRIVATE
  ${SUIT_SUBSYS_DIR}/storage/include
)

target_sources(app PRIVATE
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_encode.c
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_envelope.c
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_flags.c
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_mpi.c
  # Use nRF54H20-specific implementation.
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_nrf54h20.c
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_nvv.c
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_report.c
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_update.c
  )

target_include_directories(app PRIVATE
  ${SUIT_SUBSYS_DIR}/storage/include
  )

zephyr_compile_definitions(
  CONFIG_SUIT_STORAGE_N_UPDATE_REGIONS=7
  CONFIG_SUIT_STORAGE_N_ENVELOPES=10
  CONFIG_SUIT_STORAGE_ENVELOPE_SIZE=2048
  CONFIG_SUIT_STORAGE_CONFIG_SIZE=128
  )

# Link with the CMake target, that includes SUIT platform internal APIs header
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_update_magic_values)
