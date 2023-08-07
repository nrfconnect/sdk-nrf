# Kconfig - nRF91 THINGY91 board configuration
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ADP536X adp5360_init.c)

if(CONFIG_BOARD_THINGY91_NRF9160_NS)
	# Use static partition layout to ensure the partition layout remains
	# unchanged after DFU. This needs to be made globally available
	# because it is used in other CMake files.
	if(CONFIG_LWM2M_CARRIER)
		set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91_pm_static_lwm2m_carrier.yml CACHE INTERNAL "")
	else()
		set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/thingy91_pm_static.yml CACHE INTERNAL "")
	endif()
endif()

zephyr_library_sources(nrf52840_reset.c)
