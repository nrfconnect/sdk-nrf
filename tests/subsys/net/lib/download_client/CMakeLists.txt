#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(download_client)

set(APP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add test sources
target_sources(app PRIVATE ${APP_SRC_DIR}/main.c)
target_sources(app PRIVATE ${APP_SRC_DIR}/stubs.c)
target_sources(app PRIVATE ${APP_SRC_DIR}/socket_mock.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/../nrf/subsys/net/lib/download_client/src/download_client.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/../nrf/subsys/net/lib/download_client/src/parse.c)

# Add includes directories
target_include_directories(app PRIVATE ${APP_SRC_DIR})
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/../nrf/include/net/)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip/)

add_compile_definitions(CONFIG_COAP=1)
add_compile_definitions(CONFIG_DOWNLOAD_CLIENT_LOG_LEVEL=4)
add_compile_definitions(CONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE=256)
add_compile_definitions(CONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE=32)
add_compile_definitions(CONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE=64)
add_compile_definitions(CONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS=0)
add_compile_definitions(CONFIG_DOWNLOAD_CLIENT_BUF_SIZE=0x40)
add_compile_definitions(CONFIG_DOWNLOAD_CLIENT_STACK_SIZE=2048)
