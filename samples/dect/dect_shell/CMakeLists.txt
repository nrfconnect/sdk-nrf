#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_shell_fullstack)

target_include_directories(app PRIVATE
  src
  src/at
  src/auto_connect
  src/cloud
  src/hostname
  src/native_tls
  src/ping
  src/print
  src/utils
)

target_sources(app PRIVATE
  src/main.c
  src/shell.c
  src/auto_connect/auto_connect.c
  src/auto_connect/auto_connect_shell.c
  src/print/print_shell.c
  src/utils/desh_print.c
)

target_sources_ifdef(CONFIG_AT_CMD_CUSTOM app PRIVATE src/at/at_cmd_mode_custom.c)
target_sources_ifdef(CONFIG_SAMPLE_DESH_CLOUD_MQTT app PRIVATE src/cloud/cloud_mqtt_shell.c)
target_sources_ifdef(CONFIG_SAMPLE_DESH_CLOUD_COAP app PRIVATE src/cloud/cloud_coap_shell.c)
target_sources_ifdef(CONFIG_NET_HOSTNAME_DYNAMIC app PRIVATE src/hostname/hostname_shell.c)
target_sources_ifdef(CONFIG_SAMPLE_DESH_NATIVE_TLS app PRIVATE src/native_tls/desh_at_cmng.c)
target_sources_ifdef(CONFIG_SAMPLE_DESH_NATIVE_TLS app PRIVATE src/native_tls/desh_native_tls.c)
target_sources_ifdef(CONFIG_SAMPLE_DESH_DECT_ICMP_PING app PRIVATE src/ping/icmp_ping.c)
target_sources_ifdef(CONFIG_SAMPLE_DESH_DECT_ICMP_PING app PRIVATE src/ping/icmp_ping_shell.c)
