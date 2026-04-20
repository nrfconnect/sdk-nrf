#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
# Board/shield and overlay lookup in original directory
set(APPLICATION_CONFIG_DIR "\${ZEPHYR_BASE}/samples/net/sockets/http_get")
# Point to original prj.conf (add one more file here if you need overrides)
set(CONF_FILE "\${ZEPHYR_BASE}/samples/net/sockets/http_get/prj.conf")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(http_get)

FILE(GLOB app_sources ${ZEPHYR_BASE}/samples/net/sockets/http_get/src/*.c)
target_sources(app PRIVATE ${app_sources})

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
  app
  ${ZEPHYR_BASE}/samples/net/sockets/http_get/src/globalsign_r1.der
  ${gen_dir}/globalsign_r1.der.inc
)
