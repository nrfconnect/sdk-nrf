#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
zephyr_library()
zephyr_library_link_libraries(nrf_bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_GATT_HIDS hids.c)
zephyr_library_sources_ifdef(CONFIG_BT_GATT_HIDS_C hids_c.c)
zephyr_library_sources_ifdef(CONFIG_BT_GATT_THROUGHPUT throughput.c)
zephyr_library_sources_ifdef(CONFIG_BT_GATT_NUS nus.c)
zephyr_library_sources_ifdef(CONFIG_BT_GATT_NUS_C nus_c.c)
zephyr_library_sources_ifdef(CONFIG_BT_GATT_LBS lbs.c)
zephyr_library_sources_ifdef(CONFIG_BT_LINK_CTX_MANAGER ble_link_ctx_manager.c)
