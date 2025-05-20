#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_sources_ifdef(CONFIG_EVENT_MANAGER_PROXY app PRIVATE event_proxy_init.c)
target_sources_ifdef(CONFIG_APP_SENSOR_STUB_GEN app PRIVATE sensor_stub_gen.c)
target_sources_ifdef(CONFIG_APP_WORKLOAD_SIM app PRIVATE workload_sim.c)
