#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rt_framework)

target_include_directories(app PRIVATE include)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/rt_framework.c
  src/rt_fastpath.c
)
# NORDIC SDK APP END
