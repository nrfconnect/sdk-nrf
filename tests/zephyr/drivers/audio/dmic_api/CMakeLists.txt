# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dmic_api)

FILE(GLOB app_sources ${ZEPHYR_BASE}/tests/drivers/audio/dmic_api/src/*.c)
target_sources(app PRIVATE ${app_sources})
