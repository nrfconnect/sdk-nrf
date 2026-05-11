#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

append_with_prefix(src_crypto_core_oberon ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/platform/
  platform.c
  platform_util.c
)

append_with_prefix(src_crypto_core_oberon ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/utilities
  constant_time.c
)

append_with_prefix(src_crypto_core_oberon ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/core/
  psa_crypto.c
  psa_crypto_client.c
  psa_crypto_slot_management.c
  psa_crypto_storage.c
)

append_with_prefix(src_crypto_core_oberon ${NRF_SECURITY_DIR}/src/
  psa_crypto_driver_wrappers.c
)

if(CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
  append_with_prefix(src_crypto_core_oberon ${NRF_SECURITY_DIR}/src/
    mbedtls_psa_platform.c
  )
endif()

add_library(psa_core STATIC
    ${src_crypto_core_oberon}
)

target_compile_definitions(psa_core
  PRIVATE
    BUILDING_MBEDTLS_CRYPTO
)

target_link_libraries(psa_core
  PRIVATE
    psa_crypto_library_config
    psa_interface
    nrf_security_utils
)

nrf_security_add_zephyr_options_library(psa_core)

target_link_libraries(${mbedcrypto_target}
  PRIVATE
    psa_core
)
