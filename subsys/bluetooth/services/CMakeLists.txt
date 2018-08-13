#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_link_libraries(nrf_bluetooth)

zephyr_library_sources_ifdef(CONFIG_NRF_BT_HIDS hids.c)
zephyr_library_sources_ifdef(CONFIG_NRF_BT_THROUGHPUT throughput.c)
zephyr_library_sources(ble_link_ctx_manager.c)
