# Kconfig - nRF91 THINGY91 board configuration
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_ADP536X adp5360_init.c)
zephyr_library_sources(nrf52840_reset.c)
