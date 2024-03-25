# Kconfig - nRF91 THINGY91 board configuration
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ADP536X adp5360_init.c)

if(CONFIG_THINGY91_STATIC_PARTITIONS_FACTORY)
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91_pm_static.yml CACHE INTERNAL "")
endif()

if(CONFIG_THINGY91_STATIC_PARTITIONS_SECURE_BOOT)
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91_pm_static_secure_boot.yml CACHE INTERNAL "")
endif()

if(CONFIG_THINGY91_STATIC_PARTITIONS_LWM2M_CARRIER)
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91_pm_static_lwm2m_carrier.yml CACHE INTERNAL "")
endif()

zephyr_library_sources(nrf52840_reset.c)
