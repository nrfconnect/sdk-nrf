#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# This check is needed since CMake does not fail when performing
# 'set_source_files_properties' to a source file which does not exist.
set(pcd_dir ${ZEPHYR_NRF_MODULE_DIR}/subsys/pcd)
set(pcd_source ${pcd_dir}/src/pcd.c)
if (NOT EXISTS ${pcd_source})
  message(FATAL_ERROR "Unable to find source being tested")
endif()

# Set 'CONFIG_MCUBOOT' in order to include code which is conditionally included.
set_source_files_properties(
  ${pcd_source}
  DIRECTORY ${pcd_dir}
  PROPERTIES COMPILE_DEFINITIONS
  "CONFIG_MCUBOOT")

project(pcd_test)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})
