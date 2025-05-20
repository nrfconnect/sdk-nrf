#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PRIVATE .)
target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_shell.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_ctrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_ctrl_scheduler_integration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_scan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_rx.c
    )
add_subdirectory(common)
add_subdirectory(ping)
add_subdirectory(perf)
add_subdirectory(rf_tool)
add_subdirectory(mac)
