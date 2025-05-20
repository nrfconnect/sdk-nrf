#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PRIVATE .)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac_shell.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac_ctrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac_cluster_beacon.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac_pdu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac_nbr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac_nbr_bg_scan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_mac.c
    )
