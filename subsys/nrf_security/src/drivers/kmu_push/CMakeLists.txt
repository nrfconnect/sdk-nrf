#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(psa_interface
    INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(cracen_psa_driver
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/kmu_push.c
)
