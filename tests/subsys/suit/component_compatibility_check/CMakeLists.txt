#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_suit_component_compatibility_check)
include(../cmake/test_template.cmake)

target_include_directories(app PRIVATE
  ${SUIT_SUBSYS_DIR}/mci/include
  ${SUIT_SUBSYS_DIR}/storage/include
)

target_sources(app PRIVATE
  # Use nRF54H20-specific MCI implementation.
  ${SUIT_SUBSYS_DIR}/mci/src/suit_mci_nrf54h20.c
  ${SUIT_SUBSYS_DIR}/platform/sdfw/src/suit_plat_component_compatibility.c
  ${SUIT_SUBSYS_DIR}/storage/src/suit_storage_nrf54h20.c
)

# Link with the CMake target, that includes SUIT platform internal APIs header
zephyr_library_link_libraries(suit_utils)
zephyr_library_link_libraries(suit_storage_interface)
zephyr_library_link_libraries(suit_platform_interface)
