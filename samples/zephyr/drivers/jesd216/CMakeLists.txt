# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(flash_jesd216)

zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/flash)

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/drivers/jesd216/src/main.c)
