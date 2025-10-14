#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_REMOTE_BOARD)
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
