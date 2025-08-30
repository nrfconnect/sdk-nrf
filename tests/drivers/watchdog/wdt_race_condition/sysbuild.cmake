#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Run code on a second core if supported
if(SB_CONFIG_SECOND_CORE)
  ExternalZephyrProject_Add(
    APPLICATION remote_core
    SOURCE_DIR  ${APP_DIR}/remote
    BOARD "nrf54h20dk/nrf54h20/cpurad"
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
