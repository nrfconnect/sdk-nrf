#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

zephyr_include_directories(
  kernel/include
  ${ARCH_DIR}/${ARCH}/include
)

zephyr_library_sources(src/main.c)
