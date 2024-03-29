#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include(../cmake/test_template.cmake)

project(plat_override_image_size)

target_include_directories(testbinary PRIVATE ${SUIT_SUBSYS_DIR}/platform/include)
target_compile_options(test_interface INTERFACE -DCONFIG_SUIT_PLATFORM)

target_sources(testbinary PRIVATE
  src/main.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_components.c
  ${SUIT_SUBSYS_DIR}/platform/src/suit_plat_error_convert.c
)
