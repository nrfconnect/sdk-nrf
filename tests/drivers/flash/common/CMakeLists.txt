# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_qspi_nor)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/drivers/flash/common/src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_SOC_NRF54L15_CPUAPP)
  target_compile_definitions(app PUBLIC "TEST_AREA_OFFSET=0x00")
  message(STATUS "TEST_AREA_OFFSET=0x00")
endif()
