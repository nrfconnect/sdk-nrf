#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_basic.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_data.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_multicontext.c)

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/test_multicontext_handler.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_name_style_sorting.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_oom.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_subs.c)
