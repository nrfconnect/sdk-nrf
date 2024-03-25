# Copyright (c) 2019 Nordic Semiconductor ASA.
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_BOARD_THINGY91_NRF9160 OR CONFIG_BOARD_THINGY91_NRF9160_NS)
  board_runner_args(nrfjprog "--softreset")
  board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
elseif(CONFIG_BOARD_THINGY91_NRF52840)
  board_runner_args(jlink "--device=nrf52" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
