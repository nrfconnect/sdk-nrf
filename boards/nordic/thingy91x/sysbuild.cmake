#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_BOARD_THINGY91X_NRF9151_NS)
  # Use static partition layout to ensure the partition layout remains
  # unchanged after DFU. This needs to be made globally available
  # because it is used in other CMake files.
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf9151_pm_static.yml CACHE INTERNAL "")
endif()

if(SB_CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP OR SB_CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP_NS)
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static.yml CACHE INTERNAL "")
endif()
