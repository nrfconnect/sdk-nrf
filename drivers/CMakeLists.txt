#
# Copyright (c) 2018-2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_subdirectory_ifdef(CONFIG_SENSOR sensor)
add_subdirectory_ifdef(CONFIG_NETWORKING net)
add_subdirectory_ifdef(CONFIG_BT_LL_NRFXLIB bt_ll_nrfxlib)
add_subdirectory_ifdef(CONFIG_MPSL mpsl)
add_subdirectory_ifdef(CONFIG_NRF_802154_RADIO_DRIVER ieee802154)
add_subdirectory_ifdef(CONFIG_NRF_802154_SL nrf_802154_sl)
add_subdirectory(hw_cc310)
add_subdirectory(entropy)
add_subdirectory(gps)
