#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aws_jobs_test)

test_runner_generate(src/main.c)
cmock_handle(${ZEPHYR_BASE}/include/zephyr/net/mqtt.h)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/include/net/
  )
