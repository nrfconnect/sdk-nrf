#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_zcbor_encode)

find_program(ZCBOR zcbor REQUIRED)

set(zcbor_args
  code
  -c ${CMAKE_CURRENT_LIST_DIR}/../cddl/test.cddl
  --output-c ${PROJECT_BINARY_DIR}/src/test_encode.c
  --output-h ${PROJECT_BINARY_DIR}/include/test_encode.h
  -t Test
  -e
  )

add_custom_command(
  OUTPUT
  ${PROJECT_BINARY_DIR}/src/test_encode.c
  ${PROJECT_BINARY_DIR}/include/test_encode.h
  DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/../cddl/test.cddl
  COMMAND
  ${ZCBOR} ${zcbor_args}
  )

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources} ${PROJECT_BINARY_DIR}/src/test_encode.c)
target_include_directories(app PRIVATE ${PROJECT_BINARY_DIR}/include)
