# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_BOARD_TPM530MEVK_TPM530M_NS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file tfm_merged.hex)
endif()

board_runner_args(jlink "--device=nRF9151_xxCA" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
