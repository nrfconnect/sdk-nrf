#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include_guard(GLOBAL)

if(NOT SB_CONFIG_IMAGE_2_BOARD STREQUAL "")

  ExternalZephyrProject_Add(
    APPLICATION coremark_${SB_CONFIG_IMAGE_2_BOARD}
    SOURCE_DIR ${APP_DIR}
    BOARD ${SB_CONFIG_IMAGE_2_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

endif()

if(NOT SB_CONFIG_IMAGE_3_BOARD STREQUAL "")

  ExternalZephyrProject_Add(
    APPLICATION coremark_${SB_CONFIG_IMAGE_3_BOARD}
    SOURCE_DIR ${APP_DIR}
    BOARD ${SB_CONFIG_IMAGE_3_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

endif()
