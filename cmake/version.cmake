math(EXPR NCS_VERSION_CODE "(${NCS_VERSION_MAJOR} << 16) + (${NCS_VERSION_MINOR} << 8)  + (${NCS_VERSION_PATCH})")

# to_hex is made available by ${ZEPHYR_BASE}/cmake/hex.cmake
to_hex(${NCS_VERSION_CODE} NCS_VERSION_NUMBER)

configure_file(${ZEPHYR_NRF_MODULE_DIR}/ncs_version.h.in ${ZEPHYR_BINARY_DIR}/include/generated/ncs_version.h)
