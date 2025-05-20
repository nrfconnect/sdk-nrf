#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfu_multi_image_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Generate a test DFU Multi Image package to be parsed by a unit test to verify
# that both the package builder and the parser are compatible with each other.

file(WRITE
  ${PROJECT_BINARY_DIR}/update1.bin
  "somecontent"
  )

file(WRITE
  ${PROJECT_BINARY_DIR}/update2.bin
  "anothercontent"
  )

execute_process(
  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  COMMAND ${Python3_EXECUTABLE}
    ${ZEPHYR_NRF_MODULE_DIR}/scripts/bootloader/dfu_multi_image_tool.py
    create
    --image -1 update1.bin
    --image 1000000 update2.bin
    dfu_package.bin
  )

file(READ
  ${PROJECT_BINARY_DIR}/dfu_package.bin
  DFU_PACKAGE_HEX
  HEX
  )

target_compile_definitions(app PRIVATE
  DFU_PACKAGE_HEX="${DFU_PACKAGE_HEX}"
  )
