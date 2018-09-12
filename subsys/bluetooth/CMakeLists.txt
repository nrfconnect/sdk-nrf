#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#
add_library(nrf_bluetooth INTERFACE)

add_subdirectory_ifdef(CONFIG_NRF_BT_SVC_COMMON common)
add_subdirectory_ifdef(CONFIG_NRF_BT_HIDS services)
add_subdirectory_ifdef(CONFIG_NRF_BT_THROUGHPUT services)
add_subdirectory_ifdef(CONFIG_NRF_BT_NUS services)
