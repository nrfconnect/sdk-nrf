#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PRIVATE .)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/dect_phy_rf_tool.c
    )
