#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)

project(plat_retrieve_manifest)

target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_retrieve_manifest.c
  ${SUIT_SUBSYS_DIR}/platform/sdfw/src/suit_plat_retrieve_manifest_sdfw_specific.c
)

if (CONFIG_MOCK_SUIT_MEMORY_LAYOUT)
  target_include_directories(testbinary PRIVATE .)
endif()
