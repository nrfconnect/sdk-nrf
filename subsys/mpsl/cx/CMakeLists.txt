#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MPSL_CX_3WIRE OR
   CONFIG_MPSL_CX_1WIRE OR
   CONFIG_MPSL_CX_BT_1WIRE OR
   CONFIG_MPSL_CX_NRF700X OR
   CONFIG_MPSL_CX_SOFTWARE)
   zephyr_library()

   zephyr_library_sources_ifdef(CONFIG_MPSL_CX_3WIRE 3wire/mpsl_cx_3wire.c)
   zephyr_library_sources_ifdef(CONFIG_MPSL_CX_BT_1WIRE 1wire/mpsl_cx_1wire.c)
   zephyr_library_sources_ifdef(CONFIG_MPSL_CX_1WIRE 1wire/mpsl_cx_1wire.c)
   zephyr_library_sources_ifdef(CONFIG_MPSL_CX_NRF700X nrf700x/mpsl_cx_nrf700x.c)
   zephyr_library_sources_ifdef(CONFIG_MPSL_CX_SOFTWARE_RPC software/mpsl_cx_software_rpc.c)
endif()

if(CONFIG_MPSL_CX_NRF700X AND CONFIG_SOC_SERIES_NRF53 AND CONFIG_NRF_RPC)
  zephyr_library_sources(nrf700x/mpsl_cx_nrf700x_rpc.c)
endif()

if(CONFIG_MPSL_CX_SOFTWARE AND CONFIG_MPSL)
  zephyr_library_sources(software/mpsl_cx_software.c)
endif()
