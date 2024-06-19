#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (${NORMALIZED_BOARD_TARGET} STREQUAL "nrf54h20dk_nrf54h20_cpuapp")
  list(LENGTH SHIELD SHIELD_NUM)
  list(LENGTH machine_learning_SHIELD APP_SHIELD_NUM)
  math(EXPR SHIELD_NUM "${SHIELD_NUM} + ${APP_SHIELD_NUM}")

  if(NOT SB_CONFIG_ML_APP_INCLUDE_REMOTE_IMAGE AND SHIELD_NUM EQUAL 0)
    message(FATAL_ERROR "Missing configuration for the ${NORMALIZED_BOARD_TARGET}. It requires additional shield or snippet")
  endif()
endif()

if(SB_CONFIG_ML_APP_INCLUDE_REMOTE_IMAGE AND DEFINED SB_CONFIG_ML_APP_REMOTE_BOARD)
  # Add remote project
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${APP_DIR}/remote
    BOARD ${SB_CONFIG_ML_APP_REMOTE_BOARD}
    BOARD_REVISION ${BOARD_REVISION}
  )

if(SB_CONFIG_PARTITION_MANAGER)
  set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUNET)
  set_property(GLOBAL APPEND PROPERTY PM_CPUNET_IMAGES remote)
  set_property(GLOBAL PROPERTY DOMAIN_APP_CPUNET remote)
  set(CPUNET_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")
endif()

# Add a dependency so that the remote image will be built first.
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} ipc_radio remote)
# Add dependency so that the remote image is flashed first.
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} ipc_radio remote)

endif()
