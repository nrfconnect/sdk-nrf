#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT "${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  # Add remote project
  ExternalZephyrProject_Add(
      APPLICATION remote_sleep_forever
      SOURCE_DIR ${APP_DIR}/../common/remote_sleep_forever
      BOARD ${SB_CONFIG_REMOTE_BOARD}
      BOARD_REVISION ${BOARD_REVISION}
    )

  # Add a dependency so that the remote image will be built and flashed first
  add_dependencies(qdec remote_sleep_forever)
  # Add dependency so that the remote image is flashed first.
  sysbuild_add_dependencies(FLASH qdec remote_sleep_forever)
endif()
