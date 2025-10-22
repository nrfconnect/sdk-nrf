#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote_rad
  SOURCE_DIR ${SYSBUILD_NRF_MODULE_DIR}/tests/benchmarks/power_consumption/common/remote_sleep_forever
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
  BOARD_REVISION ${BOARD_REVISION}
)

ExternalZephyrProject_Add(
  APPLICATION remote_flpr
  SOURCE_DIR  ${APP_DIR}/remote
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuflpr
  BOARD_REVISION ${BOARD_REVISION}
)
