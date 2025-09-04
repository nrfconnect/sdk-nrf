#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_REMOTE_GLOBAL_DOMAIN_CLOCK_FREQUENCY_SWITCHING)
  set(REMOTE_SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/multicore/common/remote_gdf_switching)
else()
  set(REMOTE_SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/benchmarks/power_consumption/common/remote_sleep_forever)
endif()

# Add remote project
ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${REMOTE_SOURCE_DIR}
  BOARD ${SB_CONFIG_BOARD}/${SB_CONFIG_SOC}/cpurad
  BOARD_REVISION ${BOARD_REVISION}
)

if(SB_CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(remote PROPERTIES
  IMAGE_CONF_SCRIPT ${ZEPHYR_BASE}/share/sysbuild/image_configurations/MAIN_image_default.cmake
  )
  UpdateableImage_Add(APPLICATION remote)
endif()
