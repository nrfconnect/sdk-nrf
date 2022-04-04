#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources_ifdef(CONFIG_APP_ACK_EVENT         app PRIVATE ack_event.c)
target_sources_ifdef(CONFIG_APP_CONFIG_EVENT      app PRIVATE config_event.c)
target_sources_ifdef(CONFIG_APP_CONTROL_EVENT     app PRIVATE control_event.c)
target_sources_ifdef(CONFIG_APP_MEASUREMENT_EVENT app PRIVATE measurement_event.c)
