#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)

project(suit_plat_check_image_match)
target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLATFORM
  -DCONFIG_SUIT_STREAM_SINK_DIGEST)
target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/platform/sdfw/src/suit_plat_check_image_match_sdfw_specific.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_check_image_match.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_error_convert.c
)
