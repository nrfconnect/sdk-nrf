#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT "${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  # Add remote project
  ExternalZephyrProject_Add(
      APPLICATION remote
      SOURCE_DIR ${APP_DIR}/remote
      BOARD ${SB_CONFIG_REMOTE_BOARD}
      BOARD_REVISION ${BOARD_REVISION}
    )

  # Add a dependency so that the remote image will be built and flashed first
  add_dependencies(gpio remote)
  # Add dependency so that the remote image is flashed first.
  sysbuild_add_dependencies(FLASH gpio remote)
endif()
