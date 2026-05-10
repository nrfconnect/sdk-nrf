
if(NOT PSA_CRYPTO_EXTERNAL_CORE)
    return()
endif()

install(
  FILES
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/build_info.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_adjust_auto_enabled.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_adjust_config_key_pair_types.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_adjust_config_synonyms.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_compat.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_driver_common.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_extra.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_legacy.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_platform.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_se_driver.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_sizes.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_struct.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_types.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto_values.h
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/psa/crypto.h
      # The driver contexts are taken from nrf_security
      ${NRF_SECURITY_DIR}/include/psa/crypto_driver_contexts_key_derivation.h
      ${NRF_SECURITY_DIR}/include/psa/crypto_driver_contexts_primitives.h
      ${NRF_SECURITY_DIR}/include/psa/crypto_driver_contexts_composites.h
  DESTINATION ${INSTALL_INTERFACE_INC_DIR}/psa
)

install(
  FILES
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/mbedtls/build_info.h
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include/mbedtls/config_psa.h
  DESTINATION
    ${INSTALL_INTERFACE_INC_DIR}/mbedtls
)

install(
  FILES
    ${PSA_CRYPTO_CONFIG_INTERFACE_PATH}/${CONFIG_TF_PSA_CRYPTO_CONFIG_FILE}
  DESTINATION
    ${INSTALL_INTERFACE_INC_DIR}/
)
