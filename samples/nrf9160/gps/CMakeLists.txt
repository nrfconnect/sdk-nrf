#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gps_socket_sample)

zephyr_library_sources(src/main.c)

zephyr_library_sources_ifdef(
  CONFIG_SUPL_CLIENT_LIB
  src/supl_support.c
)
