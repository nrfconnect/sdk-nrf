#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
add_library(nrf_bluetooth INTERFACE)

if (CONFIG_NRF_BT_SVC_COMMON OR
    CONFIG_BT_GATT_DM OR
    CONFIG_BT_SCAN)
	add_subdirectory(common)
endif()

add_subdirectory_ifdef(CONFIG_BT_LL_NRFXLIB controller)

if (CONFIG_BT_GATT_HIDS   OR
    CONFIG_BT_GATT_HIDS_C OR
    CONFIG_BT_GATT_LBS    OR
    CONFIG_BT_GATT_NUS    OR
    CONFIG_BT_GATT_NUS_C  OR
    CONFIG_BT_GATT_THROUGHPUT)
	add_subdirectory(services)
endif()
