# Kconfig - nRF91 Thingy:91 X board configuration
#
# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_WIFI_NRF70)
  if(CONFIG_BOARD_THINGY91X_NRF9151 OR CONFIG_BOARD_THINGY91X_NRF9151_NS)
    zephyr_library()
    zephyr_library_sources(nrf70_support.c)
  endif()
endif()

if(CONFIG_BOARD_THINGY91X_NRF9151_NS)
  # Use static partition layout to ensure the partition layout remains
  # unchanged after DFU. This needs to be made globally available
  # because it is used in other CMake files.
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf9151_pm_static.yml CACHE INTERNAL "")
endif()

# If the BMM350 driver is not used, use a minimal initialization instead to save power.
# Once a driver is available in the Zephyr tree, this can be removed.
if (CONFIG_SENSOR AND CONFIG_REGULATOR AND CONFIG_I2C AND NOT CONFIG_BMM350)
  zephyr_library_named(bmm350_init_minimal)
  zephyr_library_sources(bmm350_init_minimal.c)
endif()

if(CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP OR CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP_NS)
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static.yml CACHE INTERNAL "")
endif()
