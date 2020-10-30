#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Building TFM requires Cmake 3.15.
cmake_minimum_required(VERSION 3.15)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_hello_world)

target_sources(app PRIVATE src/main.c)
