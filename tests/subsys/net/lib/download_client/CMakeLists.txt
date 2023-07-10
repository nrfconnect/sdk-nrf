#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(download_client)

FILE(GLOB app_sources src/mock/*.c src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app
        PRIVATE
        ${ZEPHYR_NRF_MODULE_DIR}/include/net/
        ${ZEPHYR_BASE}/subsys/net/ip/
        src/
        )

add_library(download_client STATIC
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/download_client/src/download_client.c
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/download_client/src/parse.c
        )

target_link_libraries(download_client PUBLIC zephyr_interface)
target_link_libraries(app PRIVATE download_client)

zephyr_append_cmake_library(download_client)

zephyr_compile_options(
        -DCONFIG_DOWNLOAD_CLIENT_BUF_SIZE=0x40
        -DCONFIG_DOWNLOAD_CLIENT_STACK_SIZE=2048
)

target_compile_definitions(
        download_client PRIVATE
        -DCONFIG_COAP=1
        -DCONFIG_DOWNLOAD_CLIENT_LOG_LEVEL=4
        -DCONFIG_DOWNLOAD_CLIENT_HTTP_FRAG_SIZE=256
        -DCONFIG_DOWNLOAD_CLIENT_MAX_HOSTNAME_SIZE=32
        -DCONFIG_DOWNLOAD_CLIENT_MAX_FILENAME_SIZE=64
        -DCONFIG_DOWNLOAD_CLIENT_TCP_SOCK_TIMEO_MS=0
)
