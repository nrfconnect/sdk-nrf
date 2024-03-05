#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  return()
endif()

message(STATUS "Building remote firmware for ${SB_CONFIG_REMOTE_BOARD}")

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
)

# Add a dependency so that the remote sample will be built and flashed first
add_dependencies(caf_sensor_manager remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH caf_sensor_manager remote)
