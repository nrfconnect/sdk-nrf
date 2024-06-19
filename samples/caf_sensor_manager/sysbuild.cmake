#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "" OR NOT SB_CONFIG_INCLUDE_REMOTE_IMAGE)
  return()
endif()

# This sample always requires PPR snippet for nRF54h20
if (${NORMALIZED_BOARD_TARGET} STREQUAL "nrf54h20dk_nrf54h20_cpuapp")
  if(NOT nordic-ppr IN_LIST caf_sensor_manager_SNIPPET)
    set(caf_sensor_manager_SNIPPET nordic-ppr CACHE STRING "" FORCE)
  endif()
endif()

message(STATUS "Building remote firmware for ${SB_CONFIG_REMOTE_BOARD}")

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)

if(SB_CONFIG_PARTITION_MANAGER)
  set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
  set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES remote)
  set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET remote)
  set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")
endif()

# Add a dependency so that the remote sample will be built and flashed first
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
