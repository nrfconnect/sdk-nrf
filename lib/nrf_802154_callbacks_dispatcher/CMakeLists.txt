#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_include_directories(.)

zephyr_library_sources(nrf_802154_callbacks_dispatcher.c)
zephyr_linker_sources(SECTIONS callbacks.ld)
