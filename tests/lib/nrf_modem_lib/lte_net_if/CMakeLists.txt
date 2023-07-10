#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lte_net_if)

# Turn off warnings of deprecated declarations
zephyr_compile_options(-Wno-deprecated-declarations)

# Create mocks
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/lte_lc.h FUNC_EXCLUDE ".*(lte_lc_rai_req|lte_lc_rai_param_set)")
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/nrf_modem_lib.h FUNC_EXCLUDE ".*nrf_modem_lib_shutdown_wait")
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/pdn.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_at.h FUNC_EXCLUDE ".*nrf_modem_at_scanf")
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_socket.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_errno.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_gai_errors.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_os.h)

# Generate runner for the test
test_runner_generate(src/main.c)

# Add source folder
target_include_directories(app PRIVATE src)

# Add test file
target_sources(app PRIVATE src/main.c)

# Add UUT
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/lte_net_if/lte_net_if.c)
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/lte_net_if/lte_ip_addr_helper.c)
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/nrf91_sockets.c)

target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/lte_net_if)

add_compile_definitions(CONFIG_NRF_MODEM_LIB_MEM_DIAG)
add_compile_definitions(CONFIG_NRF_MODEM_LIB_NET_IF_CONNECT_TIMEOUT_SECONDS=30)
add_compile_definitions(CONFIG_NRF_MODEM_LIB_NET_IF)
add_compile_definitions(CONFIG_NRF_MODEM_LIB_NET_IF_WORKQUEUE_STACK_SIZE=1024)
add_compile_definitions(CONFIG_NRF_MODEM_LIB_NET_IF_CONNECTION_PERSISTENCE)
