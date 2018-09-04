#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
add_library(nrf_bluetooth INTERFACE)

if (CONFIG_NRF_BT_SVC_COMMON OR
    CONFIG_NRF_BT_GATT_DB_DISCOVERY OR
    CONFIG_NRF_BT_SCAN)
	add_subdirectory(common)
endif()

add_subdirectory_ifdef(CONFIG_BT_LL_NRFXLIB controller)

if (CONFIG_NRF_BT_DIS  OR
    CONFIG_NRF_BT_HIDS OR
    CONFIG_NRF_BT_LBS  OR
    CONFIG_NRF_BT_NUS  OR
    CONFIG_NRF_BT_THROUGHPUT)
	add_subdirectory(services)
endif()
