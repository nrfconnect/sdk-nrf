#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            addr.c
            addr_expects.c
            adv.c
            adv_expects.c
            conn.c
            conn_expects.c
            crypto.c
            crypto_expects.c
            hci_core.c
            hci_core_expects.c
            kernel.c
            kernel_expects.c
            keys.c
            keys_expects.c
            net_buf.c
            net_buf_expects.c
            rpa.c
            rpa_expects.c
            scan.c
            scan_expects.c
            settings.c
            settings_expects.c
            smp.c
            smp_expects.c
)

target_include_directories(mocks PUBLIC
  ..
  ${ZEPHYR_NRF_MODULE_DIR}/include
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host
  ${ZEPHYR_NRF_MODULE_DIR}/tests/bluetooth/host
  ${ZEPHYR_NRF_MODULE_DIR}/tests/bluetooth/host/id/mocks
)

target_link_libraries(mocks PRIVATE test_interface)
