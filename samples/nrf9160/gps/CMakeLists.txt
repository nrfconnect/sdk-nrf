#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(gps_socket_sample)

zephyr_library_sources(src/main.c)

zephyr_library_sources_ifdef(
  CONFIG_SUPL_CLIENT_LIB
  src/supl_support.c
)

