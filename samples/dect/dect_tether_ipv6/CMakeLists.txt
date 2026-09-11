#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_tether_ipv6)

target_include_directories(app PRIVATE
	${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/dect/dect_tether_ipv6)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_DECT_TETHER_IPV6_SAMPLE_MDNS_DNS_SD app PRIVATE
		     src/mdns_dns_sd_listen.c)
