#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_ENABLE_CPUPPR)
  # Add remote project
  ExternalZephyrProject_Add(
    APPLICATION remote_ppr
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpuppr
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
