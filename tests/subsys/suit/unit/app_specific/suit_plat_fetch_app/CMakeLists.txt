#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)

project(plat_fetch_app)

target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/platform/app/src/suit_plat_fetch_app_specific.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_error_convert.c
)
