#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_cddl_gen)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_cddl_source(app test.cddl ENTRY_TYPES Test ENCODE
  ${VERBOSE}
  ${CANONICAL}
  )
