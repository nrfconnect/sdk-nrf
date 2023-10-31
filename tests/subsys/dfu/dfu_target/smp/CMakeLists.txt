#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dfu_target_smp_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/dfu_target/src/dfu_target_smp.c
  )

set(includes
"src/"
)

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/dfu/include
  ${includes}
  )

# Mandatory stubbed flags for building test setup
target_compile_options(app
  PRIVATE
  -DCONFIG_DFU_TARGET_LOG_LEVEL=2
  -DCONFIG_DFU_TARGET_SMP=1
  -DCONFIG_DFU_TARGET_SMP_IMAGE_LIST_SIZE=2
  )

  zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
