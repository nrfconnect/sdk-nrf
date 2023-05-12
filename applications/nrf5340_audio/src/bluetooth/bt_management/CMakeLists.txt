#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_include_directories(
    advertising
    controller_config
    dfu
    scanning
)

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/bt_mgmt.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/controller_config/bt_mgmt_ctlr_cfg.c
)

if (CONFIG_BT_CENTRAL)
	target_sources(app PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/scanning/bt_mgmt_scan_for_conn.c)
endif()

if (CONFIG_BT_BAP_BROADCAST_SINK)
target_sources(app PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/scanning/bt_mgmt_scan_for_broadcast.c)
endif()

if (CONFIG_BT_OBSERVER)
target_sources(app PRIVATE
	        ${CMAKE_CURRENT_SOURCE_DIR}/scanning/bt_mgmt_scan.c)
endif()

if (CONFIG_BT_PERIPHERAL OR CONFIG_BT_BROADCASTER)
	target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/advertising/bt_mgmt_adv.c)
endif()

if (CONFIG_AUDIO_DFU_ENABLE)
	target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/dfu/bt_mgmt_dfu.c
	)
endif()
