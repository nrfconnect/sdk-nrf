#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_cddl_gen)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(py_command
  cddl-gen
  -c ${CMAKE_CURRENT_LIST_DIR}/test.cddl
  code
  --output-cmake ${PROJECT_BINARY_DIR}/test.cmake
  -t Test
  -e
  )

execute_process(
  COMMAND ${py_command}
  COMMAND_ERROR_IS_FATAL ANY
  )

include(${PROJECT_BINARY_DIR}/test.cmake)

target_link_libraries(test PRIVATE zephyr_interface)
target_link_libraries(app PRIVATE test)
