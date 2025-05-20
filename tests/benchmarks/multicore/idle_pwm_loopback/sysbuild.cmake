#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SOC_NRF54H20)
  if(SB_CONFIG_REMOTE_GLOBAL_DOMAIN_CLOCK_FREQUENCY_SWITCHING)
    set(REMOTE_SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/multicore/common/remote_gdf_switching)
  else()
    set(REMOTE_SOURCE_DIR ${APP_DIR}/remote)
  endif()

  # Add remote project
  ExternalZephyrProject_Add(
    APPLICATION remote
    SOURCE_DIR ${REMOTE_SOURCE_DIR}
    BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
    BOARD_REVISION ${BOARD_REVISION}
  )
endif()
