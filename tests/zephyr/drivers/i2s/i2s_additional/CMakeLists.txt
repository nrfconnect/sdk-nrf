# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2s_additional)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/drivers/i2s/i2s_additional/src/*.c)
target_sources(app PRIVATE ${app_sources})
