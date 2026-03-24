#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# DECT NR+ Stack Integration Tests with Unity and CMock

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_mdm_integration_test)

# Add test source files for Unity-based integration testing
# Test source files
target_sources(app PRIVATE
    src/main.c
    src/test_dect_integration.c
    src/test_dect_utils.c
    src/mocks/mock_nrf_modem_dect_mac.c
    src/mocks/mock_modem_device.c
    src/mocks/mock_ppp_net_if.c
)

# Include all necessary directories for DECT stack
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mocks
    ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include
    ${ZEPHYR_BASE}/../nrfxlib/common/include
    ${ZEPHYR_BASE}/../nrf/drivers/dect/dect_mdm
    ${ZEPHYR_BASE}/../nrf/include/net/dect
    ${ZEPHYR_BASE}/../nrf/include/net
    ${ZEPHYR_BASE}/include/zephyr/net
    ${ZEPHYR_BASE}/subsys/net/ip
)

# Add nrfxlib include directories globally so DECT stack can find headers
target_include_directories(zephyr_interface INTERFACE
    ${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include
    ${ZEPHYR_BASE}/../nrfxlib/common/include
)

# Mock PPP net_if via linker wrap (wrap symbol provided in mock_ppp_net_if.c).
# pm_device_action_run is provided directly in the mock (no wrap) so it links when PM is not built in.
target_link_options(app PUBLIC
    -Wl,--wrap=net_if_get_first_by_type
    -Wl,--wrap=net_if_start_rs
)

# Coverage workflow is documented in README and run via Twister commands.
