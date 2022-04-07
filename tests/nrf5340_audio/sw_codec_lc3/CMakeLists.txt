#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

include(${ZEPHYR_SDK_LC3_MODULE_DIR}/CMakeLists.txt)


target_sources(app
  PRIVATE
  main.c
)
