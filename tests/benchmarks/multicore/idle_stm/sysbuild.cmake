#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote_rad
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
  BOARD_REVISION ${BOARD_REVISION}
)

ExternalZephyrProject_Add(
  APPLICATION remote_ppr
  SOURCE_DIR  ${APP_DIR}/remote
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuppr
  BOARD_REVISION ${BOARD_REVISION}
)
