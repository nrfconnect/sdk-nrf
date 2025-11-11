#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_NET_BUILD_TIME_LOG)
  zephyr_library()
  zephyr_library_sources(src/net_build_time_log.c)
endif()
