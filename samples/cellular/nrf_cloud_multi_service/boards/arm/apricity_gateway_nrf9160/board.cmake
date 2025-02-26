# Copyright (c) 2025 Nordic Semiconductor ASA.
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
board_runner_args(nrfjprog "--softreset")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
