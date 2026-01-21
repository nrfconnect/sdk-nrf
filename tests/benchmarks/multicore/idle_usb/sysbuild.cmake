#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_REMOTE_BOARD)
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/power_consumption/common/remote_sleep_forever
    BOARD ${SB_CONFIG_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
