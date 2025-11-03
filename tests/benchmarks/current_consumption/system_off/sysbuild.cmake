#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SOC_NRF54H20)
  # Add remote project
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${ZEPHYR_BASE}/samples/boards/nordic/system_off/remote
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
