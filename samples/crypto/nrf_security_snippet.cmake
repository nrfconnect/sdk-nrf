
# Use "tfm_ZEPHYR_DOTCONFIG.config" and "tfm/zephyr/autoconf.h" to
# configure nrf_security on the secure side instead of using the same
# configuration on both sides.
set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DNRF_SECURITY_SETTINGS=\"ZEPHYR_DOTCONFIG=${CMAKE_CURRENT_LIST_DIR}/secure_zephyr.config
GCC_M_CPU=cortex-m33 ARM_MBEDTLS_PATH=${NCS_DIR}/mbedtls
ZEPHYR_AUTOCONF=${CMAKE_CURRENT_LIST_DIR}/secure/zephyr/autoconf.h\"
)
