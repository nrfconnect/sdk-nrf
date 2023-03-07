#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_MPSL_CX_THREAD OR CONFIG_MPSL_CX_BT_1WIRE OR CONFIG_MPSL_CX_NRF700X)
    zephyr_library()

    zephyr_library_sources_ifdef(CONFIG_MPSL_CX_THREAD thread/mpsl_cx_thread.c)
    zephyr_library_sources_ifdef(CONFIG_MPSL_CX_BT_1WIRE bluetooth/mpsl_cx_1w_bluetooth.c)
    zephyr_library_sources_ifdef(CONFIG_MPSL_CX_NRF700X nrf700x/mpsl_cx_nrf700x.c)
endif()

if (CONFIG_MPSL_CX_NRF700X AND CONFIG_SOC_SERIES_NRF53X AND CONFIG_NRF_RPC)
    zephyr_library_sources(nrf700x/mpsl_cx_nrf700x_rpc.c)
endif()
