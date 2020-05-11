# Kconfig - nRF91 THINGY91 board configuration
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

if(CONFIG_BOARD_THINGY91_NRF9160 AND NOT DEFINED CONFIG_MCUBOOT)
	zephyr_library()
	zephyr_library_sources(board_secure.c)
endif()

if(CONFIG_BOARD_THINGY91_NRF9160NS)
	zephyr_library()
	zephyr_library_sources(board_nonsecure.c)
endif()
