#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

set(spm_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/spm.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(udp)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

zephyr_include_directories(src)
