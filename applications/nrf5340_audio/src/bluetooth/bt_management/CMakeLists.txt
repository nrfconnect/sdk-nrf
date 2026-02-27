#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_include_directories(
  advertising
  scanning
  ${ZEPHYR_BASE}/subsys/bluetooth/host/
)

target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/bt_mgmt.c
)

if(CONFIG_BT_CENTRAL)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/scanning/bt_mgmt_scan_for_conn.c)
endif()

if(CONFIG_BT_BAP_BROADCAST_SINK)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/scanning/bt_mgmt_scan_for_broadcast.c)
endif()

if(CONFIG_BT_OBSERVER)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/scanning/bt_mgmt_scan.c)
endif()

if(CONFIG_BT_PERIPHERAL OR CONFIG_BT_BROADCASTER)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/advertising/bt_mgmt_adv.c)
endif()
