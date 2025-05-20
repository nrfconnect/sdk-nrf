#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_REMOTE_BOARD)
  # Add remote project
  ExternalZephyrProject_Add(
      APPLICATION remote
      SOURCE_DIR ${APP_DIR}/remote
      BOARD ${SB_CONFIG_REMOTE_BOARD}
      BOARD_REVISION ${BOARD_REVISION}
    )
  # Add a dependency so that the remote image will be built and flashed first
  add_dependencies(twim_suspend remote)
  # Add dependency so that the remote image is flashed first.
  sysbuild_add_dependencies(FLASH twim_suspend remote)
endif()
