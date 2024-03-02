#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# CMakeLists.txt - iperf3 NRF integration

zephyr_interface_library_named(iperf3)

target_include_directories(iperf3 INTERFACE .)

zephyr_library()

zephyr_library_sources(
  dscp.c
  iperf_api.c
  iperf_auth.c
  iperf_client_api.c
  iperf_error.c
  iperf_locale.c
  iperf_main.c
  iperf_sctp.c
  iperf_server_api.c
  iperf_tcp.c
  iperf_time.c
  iperf_udp.c
  iperf_util.c
  net.c
  tcp_info.c
  timer.c
  units.c
)
