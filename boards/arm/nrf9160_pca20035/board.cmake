# Copyright (c) 2019 Nordic Semiconductor ASA.
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

board_runner_args(nrfjprog "--nrf-family=NRF91" "--softreset")
board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
