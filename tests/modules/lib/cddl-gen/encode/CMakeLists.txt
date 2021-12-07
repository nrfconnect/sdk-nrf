#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_cddl_gen_encode)

find_program(CDDL_GEN cddl-gen REQUIRED)

set(cddl_gen_args
  -c ${CMAKE_CURRENT_LIST_DIR}/../cddl/test.cddl
  code
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
  ${CDDL_GEN} ${cddl_gen_args}
  )

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources} ${PROJECT_BINARY_DIR}/src/test_encode.c)
target_include_directories(app PRIVATE ${PROJECT_BINARY_DIR}/include)
