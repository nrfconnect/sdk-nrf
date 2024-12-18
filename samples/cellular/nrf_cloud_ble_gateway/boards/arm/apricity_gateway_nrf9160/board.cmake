# Copyright (c) 2019 Nordic Semiconductor ASA.
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

if(CONFIG_BOARD_APRICITY_GATEWAY_NRF9160_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

board_runner_args(jlink "--device=nRF9160_xxAA" "--speed=4000")
board_runner_args(nrfjprog "--softreset")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
