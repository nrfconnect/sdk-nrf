#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory_ifdef(CONFIG_BENCHMARK_ZIGBEE zigbee)

zephyr_sources(
  benchmark_util.c
  cli_benchmark.c
  cli_suppress.c
)

zephyr_include_directories(include)
