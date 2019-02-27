#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/data_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/multicontext_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/order_event.c)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_events.c)
