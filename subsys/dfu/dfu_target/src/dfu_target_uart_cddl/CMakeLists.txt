#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(py_dfu_uart_gen_cbor
    python3
    ${ZEPHYR_ZCBOR_MODULE_DIR}/zcbor/zcbor.py
    code
    -c ${CMAKE_CURRENT_SOURCE_DIR}/dfu_target_uart.cddl
    --output-cmake ${PROJECT_BINARY_DIR}/dfu_target_uart.cmake
    -t DFU_UART_target_Envelope DFU_UART_target_RemoteFnc DFU_UART_target_RemoteResp
    -d
    -e
    --short-names
    )

execute_process(
    COMMAND ${py_dfu_uart_gen_cbor}
    COMMAND_ERROR_IS_FATAL ANY
)
include(${PROJECT_BINARY_DIR}/dfu_target_uart.cmake)

zephyr_include_directories(
    ${ZEPHYR_ZCBOR_MODULE_DIR}/include
    ${PROJECT_BINARY_DIR}/include
)
target_link_libraries(dfu_target_uart PRIVATE zephyr_interface)
target_link_libraries(app PRIVATE dfu_target_uart)
