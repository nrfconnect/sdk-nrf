# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh_dfu_distributor)

set(common_dir ${CMAKE_CURRENT_SOURCE_DIR}/../common/src)

include_directories(${common_dir})

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources}
			${common_dir}/dfu_target.c)
