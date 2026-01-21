#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR
          "Target ${BOARD} not supported for this sample. "
          "There is no remote board selected in Kconfig.sysbuild")
endif()

ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)
set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES remote)
set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET remote)
set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")

# Add a dependency so that the remote image will be built and flashed first
add_dependencies(ipc_latency remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH ipc_latency remote)
