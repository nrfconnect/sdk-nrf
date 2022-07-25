#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(include)
zephyr_sources(benchmark_util.c
  cli_benchmark.c
  cli_suppress.c
)
