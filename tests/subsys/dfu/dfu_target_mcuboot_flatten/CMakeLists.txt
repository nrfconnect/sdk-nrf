#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfu_target_mcuboot_flatten_test)

target_sources(app PRIVATE
  src/main.c
  src/flatten_fake.c
)

target_include_directories(app PRIVATE
  src
)
