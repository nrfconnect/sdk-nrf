#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_REMOTE_BOARD)
  ExternalZephyrProject_Add(
    APPLICATION remote_sleep_forever
    SOURCE_DIR ${APP_DIR}/../common/remote_sleep_forever
    BOARD ${SB_CONFIG_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

  add_dependencies(spi remote_sleep_forever)
  sysbuild_add_dependencies(FLASH spi remote_sleep_forever)
endif()
