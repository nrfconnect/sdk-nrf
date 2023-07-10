#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf91_sockets_test)

target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)

cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_socket.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_os.h)

# add unit under test
target_sources(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/nrf91_sockets.c)

# manually add Kconfig definitions introduced by NRF_MODEM_LIB and used
# by the unit under test, but not included since we aren't enabling
# CONFIG_NRF_MODEM_LIB
add_compile_definitions(CONFIG_NRF91_SOCKET_BLOCK_LIMIT=2048)
add_compile_definitions(CONFIG_NRF_MODEM_LIB_SENDMSG_BUF_SIZE=8)

# generate runner for the test
test_runner_generate(src/nrf91_sockets_test.c)

# add test file
target_sources(app PRIVATE src/nrf91_sockets_test.c)
