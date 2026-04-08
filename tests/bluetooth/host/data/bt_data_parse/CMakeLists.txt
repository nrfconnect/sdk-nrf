# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr COMPONENTS unittest HINTS $ENV{ZEPHYR_BASE})

project(bt_data_parse)

add_subdirectory(${ZEPHYR_NRF_MODULE_DIR}/tests/bluetooth/host host_mocks)

target_link_libraries(testbinary PRIVATE host_mocks)

target_sources(testbinary
    PRIVATE
    src/main.c

    ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host/host/data.c
    ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host/common/bt_str.c
    ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host/host/uuid.c
    ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
    ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
)
