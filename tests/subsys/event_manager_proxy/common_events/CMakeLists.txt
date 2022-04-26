#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources_ifdef(CONFIG_APP_DATA_EVENT   app PRIVATE data_events.c)
target_sources_ifdef(CONFIG_APP_SIMPLE_EVENT app PRIVATE simple_events.c)
target_sources_ifdef(CONFIG_APP_TEST_EVENT   app PRIVATE test_events.c)
