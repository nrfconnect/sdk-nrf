#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
zephyr_library_sources_ifdef(CONFIG_NET_L2_DECT
  dect_net_l2.c
  dect_net_l2_ipv6.c
)
zephyr_library_sources_ifdef(CONFIG_NET_L2_DECT_MGMT dect_net_l2_mgmt.c)
zephyr_library_sources_ifdef(CONFIG_NET_CONNECTION_MANAGER dect_net_l2_conn_mgr.c)
zephyr_library_sources_ifdef(CONFIG_NET_L2_DECT_BR dect_net_l2_sink.c)

# For net_private.h
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
