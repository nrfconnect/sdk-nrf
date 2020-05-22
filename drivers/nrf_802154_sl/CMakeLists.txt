#
# Copyright (c) 2019 - 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Files on src/platform are provided as sources and provide glue layer between nrf_802154_sl and zephyr
zephyr_library_named(nrf_802154_sl_platform)
zephyr_library_sources(
    src/platform/clock/nrf_802154_clock_zephyr.c
    src/platform/gpiote/nrf_802154_gpiote_crit_sect.c
    src/platform/gpiote/nrf_802154_gpiote_zephyr.c
    src/platform/hp_timer/nrf_802154_hp_timer.c
    src/platform/lp_timer/nrf_802154_lp_timer.c
    src/platform/irq/nrf_802154_irq_zephyr.c
)
