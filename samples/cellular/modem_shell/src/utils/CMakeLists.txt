#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mosh_print.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/net_utils.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/str_utils.c)
