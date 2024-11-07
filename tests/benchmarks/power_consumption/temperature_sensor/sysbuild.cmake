#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "REMOTE_BOARD must be set to a valid board name")
endif()

ExternalZephyrProject_Add(
  APPLICATION remote_sleep_forever
  SOURCE_DIR ${APP_DIR}/../common/remote_sleep_forever
  BOARD ${SB_CONFIG_REMOTE_BOARD}
)

add_dependencies(temperature_sensor remote_sleep_forever)
sysbuild_add_dependencies(FLASH temperature_sensor remote_sleep_forever)
