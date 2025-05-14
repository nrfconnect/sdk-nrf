# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP)
  board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
elseif(CONFIG_SOC_NRF54LV10A_ENGA_CPUFLPR)
  board_runner_args(jlink "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
