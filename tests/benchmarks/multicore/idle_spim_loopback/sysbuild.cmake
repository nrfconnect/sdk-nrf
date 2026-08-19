#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(TEST_REMOTE_SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/power_consumption/common/remote_sleep_forever)

if(SB_CONFIG_SOC_NRF54H20)
  # On nrf54h20, no matter if APP or PPR, build remote_sleep_forever on Radio
  set(TEST_REMOTE_BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad)
elseif(SB_CONFIG_SOC_NRF9251)
  # On nrf9251 use PPR instead of Radio
  set(TEST_REMOTE_BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuppr)
endif()

if(NOT SB_CONFIG_SOC_NRF9251_CPUPPR)
  # Add remote project unless test runs on nrf9251 PPR
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${TEST_REMOTE_SOURCE_DIR}
    BOARD ${TEST_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
