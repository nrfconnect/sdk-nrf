#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf-cloud-coap-client)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})
find_program(ZCBOR zcbor REQUIRED)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c src/handle_fota.c)
# NORDIC SDK APP END

zephyr_include_directories(src)
