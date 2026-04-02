#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
            mocks/kernel.c
            mocks/hci_core.c
            mocks/hci_core_expects.c
            mocks/aes.c
            mocks/aes_expects.c
            mocks/prng.c
            mocks/prng_expects.c

            ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host/common/bt_str.c
            ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host/host/crypto_psa.c
            ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host/host/uuid.c
            ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
)

target_include_directories(mocks PUBLIC
  .
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/host
  ${ZEPHYR_NRF_MODULE_DIR}/tests/bluetooth/host
  ${ZEPHYR_NRF_MODULE_DIR}/tests/bluetooth/host/crypto/mocks
  ${ZEPHYR_MBEDTLS_MODULE_DIR}/include
)

target_link_libraries(mocks PRIVATE test_interface)
