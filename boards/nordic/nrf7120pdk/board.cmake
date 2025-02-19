# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_SOC_NRF7120_ENGA_CPUAPP)
  board_runner_args(jlink "--device=cortex-m33" "--speed=4000")
elseif(CONFIG_SOC_NRF7120_ENGA_CPUFLPR)
  board_runner_args(jlink "--speed=4000")
endif()

if(BOARD_NRF7120PDK_NRF7120_CPUAPP_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
