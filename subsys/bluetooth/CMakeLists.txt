#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#
add_library(nrf_bluetooth INTERFACE)

add_subdirectory_ifdef(CONFIG_NRF_BT_SVC_COMMON common)

if (CONFIG_NRF_BT_DIS  OR
    CONFIG_NRF_BT_HIDS OR
    CONFIG_NRF_BT_LBS  OR
    CONFIG_NRF_BT_NUS  OR
    CONFIG_NRF_BT_THROUGHPUT)
	add_subdirectory(services)
endif()
