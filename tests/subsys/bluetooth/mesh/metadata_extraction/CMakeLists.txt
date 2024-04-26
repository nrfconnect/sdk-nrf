#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bt_mesh_metadata_extraction_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(script_args ${PROJECT_BINARY_DIR})

if(CONFIG_COMP_DATA_LAYOUT_SINGLE)
  list(PREPEND script_args --single)
else()
  list(PREPEND script_args --multiple)
endif()

list(APPEND script_args childparent)

if(NOT SYSBUILD)
  add_custom_target(verify_metadata ALL
    ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/verify_metadata.py ${script_args}
    DEPENDS parse_mesh_metadata
  )
endif()
