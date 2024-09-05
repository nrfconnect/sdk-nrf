
if(NOT PSA_CRYPTO_EXTERNAL_CORE)
    return()
endif()

install(
  FILES
      ${OBERON_PSA_CORE_PATH}/include/psa/build_info.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_adjust_auto_enabled.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_adjust_config_key_pair_types.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_adjust_config_synonyms.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_compat.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_driver_common.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_driver_contexts_composites.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_driver_contexts_key_derivation.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_driver_contexts_primitives.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_extra.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_legacy.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_platform.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_se_driver.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_sizes.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_struct.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_types.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto_values.h
      ${OBERON_PSA_CORE_PATH}/include/psa/crypto.h
  DESTINATION ${INSTALL_INTERFACE_INC_DIR}/psa
)

install(
  FILES
    ${OBERON_PSA_CORE_PATH}/include/mbedtls/build_info.h
    ${OBERON_PSA_CORE_PATH}/include/mbedtls/config_psa.h
  DESTINATION 
    ${INSTALL_INTERFACE_INC_DIR}/mbedtls
)