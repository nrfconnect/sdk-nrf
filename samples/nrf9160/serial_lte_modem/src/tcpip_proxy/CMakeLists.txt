#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
target_sources_ifdef(CONFIG_SLM_TCP_PROXY app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/slm_at_tcp_proxy.c)
target_sources_ifdef(CONFIG_SLM_UDP_PROXY app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/slm_at_udp_proxy.c)
