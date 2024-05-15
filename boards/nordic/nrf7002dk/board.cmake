# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NS OR CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NS_NRF7001)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if(CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP OR CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NS
   OR CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NRF7001 OR CONFIG_BOARD_NRF7002DK_NRF5340_CPUAPP_NS_NRF7001)
  board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
endif()

if(CONFIG_TFM_FLASH_MERGED_BINARY)
  set_property(TARGET runners_yaml_props_target PROPERTY hex_file "${CMAKE_BINARY_DIR}/tfm_merged.hex")
endif()

if(CONFIG_BOARD_NRF7002DK_NRF5340_CPUNET OR CONFIG_BOARD_NRF7002DK_NRF5340_CPUNET_NRF7001)
  board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
