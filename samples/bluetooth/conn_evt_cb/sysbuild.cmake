#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (${SB_CONFIG_BOARD_NRF5340DK_NRF5340_CPUNET})
  ExternalZephyrProject_Add(
      APPLICATION empty_app_core
      SOURCE_DIR  ${ZEPHYR_NRF_MODULE_DIR}/samples/nrf5340/empty_app_core
      BOARD nrf5340dk/nrf5340/cpuapp
    )
endif()
