#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE pelion_fcc_err.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
