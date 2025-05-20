#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_SOC_NRF54H20)
  # Add remote project
  ExternalZephyrProject_Add(
      APPLICATION remote
      SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/power_consumption/common/remote_sleep_forever
      BOARD ${SB_CONFIG_REMOTE_BOARD}
      BOARD_REVISION ${BOARD_REVISION}
    )
  # Add a dependency so that the remote image will be built and flashed first
  add_dependencies(nfc_idle remote)
  # Add dependency so that the remote image is flashed first.
  sysbuild_add_dependencies(FLASH nfc_idle remote)
endif()
