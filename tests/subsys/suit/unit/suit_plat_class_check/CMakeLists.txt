#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)

project(suit_plat_class_check)

target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_class_check.c
  ${SUIT_SUBSYS_DIR}/platform/sdfw/src/suit_plat_manifest_info.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_error_convert.c
)
