#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library_named(dect_tether_ipv6_lib)
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/net/ip)
zephyr_library_sources(dect_tether_ipv6.c)
zephyr_library_sources(dect_tether_ipv6_addr.c)
zephyr_library_sources(dect_tether_ipv6_host.c)
zephyr_library_sources(dect_tether_ipv6_uplink.c)
zephyr_library_sources_ifdef(CONFIG_DECT_TETHER_IPV6_RA dect_tether_ipv6_ra.c)
zephyr_library_sources_ifdef(CONFIG_DECT_TETHER_IPV6_DHCPV6_SERVER
			     dect_tether_ipv6_dhcpv6_srv.c)
zephyr_library_sources(dect_tether_ipv6_fwd.c)
zephyr_library_sources_ifdef(CONFIG_DECT_TETHER_IPV6_MDNS_FORWARD
			     dect_tether_ipv6_mdns_fwd.c)
