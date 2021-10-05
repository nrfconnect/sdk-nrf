# Kconfig - nRF91 THINGY91 board configuration
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_BOARD_THINGY91_NRF9160 AND NOT DEFINED CONFIG_MCUBOOT)
	zephyr_library()
	zephyr_library_sources(board_secure.c)
endif()

if(CONFIG_BOARD_THINGY91_NRF9160_NS)
	zephyr_library()
	zephyr_library_sources(board_nonsecure.c)

	# Use static partition layout to ensure the partition layout remains
	# unchanged after DFU. This needs to be made globally available
	# because it is used in other CMake files.
	set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/pm_static.yml CACHE INTERNAL "")
endif()

zephyr_library_sources(nrf52840_reset.c)
