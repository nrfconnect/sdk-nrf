#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_NRF_802154_RADIO_CONFIG)
  zephyr_library()

  zephyr_library_sources(
    nrf_802154_configurator.c
    )
endif()
