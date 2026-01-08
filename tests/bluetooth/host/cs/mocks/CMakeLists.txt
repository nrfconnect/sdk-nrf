# SPDX-License-Identifier: Apache-2.0

add_library(mocks STATIC
            conn.c
            hci_core.c
            net_buf.c
)

target_include_directories(mocks PUBLIC
  ..
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host
  ${ZEPHYR_NRF_MODULE_DIR}/tests/bluetooth/host
  ${ZEPHYR_NRF_MODULE_DIR}/tests/bluetooth/host/cs/mocks
)

target_link_libraries(mocks PRIVATE test_interface)
