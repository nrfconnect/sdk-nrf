#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PRIVATE .)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_app_time.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_common_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_common_rx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_api_scheduler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_common_settings.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_common_pdu.c
    )
