#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_tether_ipv6_integration_test)

set(DECT_INT_TEST_DIR ${ZEPHYR_NRF_MODULE_DIR}/tests/drivers/dect/dect_mdm/integration)

target_sources(app PRIVATE
	src/main.c
	src/tether_dect_events.c
	src/tether_test_eth.c
	src/tether_net_fixture.c
	src/tether_host_sim.c
	${DECT_INT_TEST_DIR}/src/mocks/mock_nrf_modem_dect_mac.c
	${DECT_INT_TEST_DIR}/src/test_dect_utils.c
)

target_include_directories(app PRIVATE
	src
	${DECT_INT_TEST_DIR}/src
	${DECT_INT_TEST_DIR}/src/mocks
	${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include
	${ZEPHYR_NRF_MODULE_DIR}/drivers/dect/dect_mdm
	${ZEPHYR_NRF_MODULE_DIR}/include/net/dect
	${ZEPHYR_NRF_MODULE_DIR}/include/net
	${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/dect/tether_ipv6
	${ZEPHYR_BASE}/include/zephyr/net
	${ZEPHYR_BASE}/subsys/net/ip
)

target_include_directories(zephyr_interface INTERFACE
	${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include
)

target_link_options(app PUBLIC
	-Wl,--wrap=net_if_start_rs
)
