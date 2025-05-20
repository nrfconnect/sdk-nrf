#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Use static partition layout to ensure the partition layout remains
# unchanged after DFU. This needs to be made globally available
# because it is used in other CMake files.

# nRF91 with TF-M (default)
if(SB_CONFIG_BOARD_THINGY91X_NRF9151_NS)
  if(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_FACTORY)
    # most common configuration, nRF91 uses external flash
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf9151_pm_static.yml CACHE INTERNAL "")
  elseif(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_NRF53_EXTERNAL_FLASH)
    # special config where nRF91 is not using external flash
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf9151_pm_static_no_ext_flash.yml CACHE INTERNAL "")
  endif()
endif()

# nRF91 without TF-M (special use)
if(SB_CONFIG_BOARD_THINGY91X_NRF9151)
  if(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_FACTORY)
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf9151_pm_static_no_tfm.yml CACHE INTERNAL "")
  elseif(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_NRF53_EXTERNAL_FLASH)
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf9151_pm_static_no_ext_flash_no_tfm.yml CACHE INTERNAL "")
  endif()
endif()

# nRF53 without TF-M (default)
if(SB_CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP)
  if(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_FACTORY)
    # most common configuration, nRF53 is not using external flash
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static_no_tfm.yml CACHE INTERNAL "")
  endif()
  if(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_NRF53_EXTERNAL_FLASH)
    # special config where nRF53 is using external flash
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static_ext_flash_no_tfm.yml CACHE INTERNAL "")
  endif()
endif()

# nRF53 with TF-M (special use)
if(SB_CONFIG_BOARD_THINGY91X_NRF5340_CPUAPP_NS)
  if(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_FACTORY)
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static.yml CACHE INTERNAL "")
  endif()
  if(SB_CONFIG_THINGY91X_STATIC_PARTITIONS_NRF53_EXTERNAL_FLASH)
    set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91x_nrf5340_pm_static_ext_flash.yml CACHE INTERNAL "")
  endif()
endif()
