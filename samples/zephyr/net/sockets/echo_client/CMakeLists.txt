#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
# Board/shield and overlay lookup in original directory
set(APPLICATION_CONFIG_DIR "\${ZEPHYR_BASE}/samples/net/sockets/echo_client")
# Point to original prj.conf (add one more file here if you need overrides)
set(CONF_FILE "\${ZEPHYR_BASE}/samples/net/sockets/echo_client/prj.conf")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sockets_echo_client)

target_sources(app PRIVATE
  ${ZEPHYR_BASE}/samples/net/sockets/echo_client/src/echo-client.c
)
target_sources_ifdef(CONFIG_NET_UDP app PRIVATE
  ${ZEPHYR_BASE}/samples/net/sockets/echo_client/src/udp.c
)
target_sources_ifdef(CONFIG_NET_TCP app PRIVATE
  ${ZEPHYR_BASE}/samples/net/sockets/echo_client/src/tcp.c
)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
  app
  ${ZEPHYR_BASE}/samples/net/sockets/echo_client/src/echo-apps-cert.der
  ${gen_dir}/echo-apps-cert.der.inc
)
