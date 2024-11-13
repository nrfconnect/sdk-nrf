#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(downloader)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

test_runner_generate(src/main.c)

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/src/downloader.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/src/dl_socket.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/src/dl_parse.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/src/sanity.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/src/transports/coap.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/src/transports/http.c
)

zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/net/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/include/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/ip/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/lib/sockets)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

zephyr_linker_sources(RODATA ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/downloader/dl_transports.ld)

target_compile_options(app
  PRIVATE
  -DCONFIG_DOWNLOADER_MAX_HOSTNAME_SIZE=256
  -DCONFIG_DOWNLOADER_MAX_FILENAME_SIZE=256
  -DCONFIG_DOWNLOADER_TRANSPORT_PARAMS_SIZE=128
  -DCONFIG_DOWNLOADER_STACK_SIZE=2048
  -DCONFIG_NET_SOCKETS_POSIX_NAMES=y
  -DCONFIG_NET_IPV6=y
  -DCONFIG_NET_IPV4=y
  -DCONFIG_COAP_MAX_RETRANSMIT=2
  -DCONFIG_COAP_INIT_ACK_TIMEOUT_MS=100
  -DCONFIG_COAP_BACKOFF_PERCENT=5
  -DCONFIG_COAP_BLOCK_SIZE=5
)
