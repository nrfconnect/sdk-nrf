#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_CORE_PATH}/library/
  platform.c
  platform_util.c
)

append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_CORE_PATH}/library/
  psa_crypto.c
  psa_crypto_client.c
  psa_crypto_slot_management.c
  psa_crypto_storage.c
)

add_library(psa_core STATIC
    ${src_crypto_core_oberon}
)

# Add the nordic version of psa_crypto_driver_wrappers with the core (out of tree)
target_sources(psa_core
  PRIVATE
    ${NRF_SECURITY_ROOT}/src/psa_crypto_driver_wrappers.c
)

if(CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
  target_sources(psa_core
    PRIVATE
      ${NRF_SECURITY_ROOT}/src/mbedtls_psa_platform.c
  )
endif()

target_link_libraries(psa_core
  PRIVATE
    psa_crypto_library_config
    psa_interface
)

target_compile_definitions(psa_core
  PRIVATE
    BUILDING_MBEDTLS_CRYPTO
)

target_link_libraries(psa_core
  PRIVATE
    psa_crypto_library_config
    mbedcrypto_base
    nrf_security_utils
)

nrf_security_add_zephyr_options_library(psa_core)

target_link_libraries(${mbedcrypto_target}
  PRIVATE
    psa_core
)
