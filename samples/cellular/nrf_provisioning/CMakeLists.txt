#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nRF_Provisioning_Sample)

zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})
zephyr_include_directories(include/)

target_sources(app PRIVATE src/main.c)
