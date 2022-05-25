#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(app PRIVATE common_utils.c)
target_sources_ifdef(CONFIG_TEST app PRIVATE test_utils.c)
