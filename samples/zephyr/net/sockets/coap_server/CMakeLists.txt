#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
# Board/shield and overlay lookup in original directory
set(APPLICATION_CONFIG_DIR "\${ZEPHYR_BASE}/samples/net/sockets/coap_server")
# Point to original prj.conf (add one more file here if you need overrides)
set(CONF_FILE "\${ZEPHYR_BASE}/samples/net/sockets/coap_server/prj.conf")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coap_server)

FILE(GLOB app_sources ${ZEPHYR_BASE}/samples/net/sockets/coap_server/src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)

zephyr_linker_sources(DATA_SECTIONS
  ${ZEPHYR_BASE}/samples/net/sockets/coap_server/sections-ram.ld
)
zephyr_iterable_section(
  NAME coap_resource_coap_server
  GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT}
)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(inc_file
  ca.der
  server.der
  server_privkey.der
  coaps-server-cert.der
  coaps-server-key.der
)
  generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/sockets/coap_server/src/certs/${inc_file}
    ${gen_dir}/${inc_file}.inc
  )
endforeach()
