#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SOC_NRF54H20)
  set(REMOTE_SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/power_consumption/common/remote_sleep_forever)

  # Add remote project
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${REMOTE_SOURCE_DIR}
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
