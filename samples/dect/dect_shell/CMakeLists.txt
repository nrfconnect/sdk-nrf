#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_shell_fullstack)
# To get the app name to cloud
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/shell.c)

add_subdirectory(src/at)
add_subdirectory(src/auto_connect)
add_subdirectory(src/cloud)
add_subdirectory_ifdef(CONFIG_NET_HOSTNAME_DYNAMIC src/hostname)
add_subdirectory_ifdef(CONFIG_DESH_NATIVE_TLS src/native_tls)
add_subdirectory_ifdef(CONFIG_DESH_DECT_ICMP_PING src/ping)
add_subdirectory(src/print)
add_subdirectory(src/utils)
