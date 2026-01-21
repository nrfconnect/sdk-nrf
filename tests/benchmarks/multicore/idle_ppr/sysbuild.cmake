#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

ExternalZephyrProject_Add(
  APPLICATION remote_ppr
  SOURCE_DIR  ${APP_DIR}/remote
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuppr
  BOARD_REVISION ${BOARD_REVISION}
)
