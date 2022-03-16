#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include . stub)

zephyr_library()

zephyr_library_sources_ifdef(
  CONFIG_BENCHMARK_ZIGBEE
  benchmark_util.c
  cli_benchmark.c
  # cli_cpu.c
  # cli_remote.c
  cli_suppress.c
  zigbee/zigbee_benchmark.c
  zigbee/zigbee_protocol.c
  zigbee/zigbee_timer.c
  zigbee/zigbee_zcl_commands.c
  # zigbee/zigbee_bm_cli.c
  zigbee/zigbee_commands.c
  stub/stub_imp.c
)

zephyr_library_link_libraries(zboss)
