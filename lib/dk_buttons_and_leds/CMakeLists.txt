#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(dk_buttons_and_leds.c)

if(CONFIG_NRF52_DBG_PINS)
zephyr_library_sources(nrf52_dbg_pins.c)
endif()