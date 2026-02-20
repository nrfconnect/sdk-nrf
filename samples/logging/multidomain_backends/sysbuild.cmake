#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(${NORMALIZED_BOARD_TARGET} STREQUAL "nrf54lm20dk_nrf54lm20a_cpuapp")
  if(NOT nordic-flpr IN_LIST multidomain_backends_SNIPPET)
    set(multidomain_backends_SNIPPET nordic-flpr CACHE STRING "" FORCE)
  endif()
endif()

if("${SB_CONFIG_REMOTE_BOARD}" STREQUAL "")
  message(FATAL_ERROR "Target ${BOARD} not supported for this sample. "
    "There is no remote board selected in Kconfig.sysbuild")
endif()

ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR  ${APP_DIR}/remote
  BOARD       ${SB_CONFIG_REMOTE_BOARD}
  BOARD_REVISION ${BOARD_REVISION}
)

sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)
