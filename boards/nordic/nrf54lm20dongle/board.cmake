# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Note: nRF54LM20A is used intentionally even though only nRF54LM20B is supported by the board.
# This is because JLink does not support nRF54LM20B yet.
if(CONFIG_SOC_NRF54LM20B_CPUAPP)
  board_runner_args(jlink "--device=nRF54LM20A_M33" "--speed=4000")
elseif(CONFIG_SOC_NRF54LM20B_CPUFLPR)
  board_runner_args(jlink "--device=nRF54LM20A_RV32" "--speed=4000")
endif()

if(CONFIG_TRUSTED_EXECUTION_NONSECURE)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
