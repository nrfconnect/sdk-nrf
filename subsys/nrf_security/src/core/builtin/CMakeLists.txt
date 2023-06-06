#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add generated path first in list (order dependent)
target_include_directories(mbedcrypto_common
    INTERFACE
      ${generated_include_path}
      ${NRF_SECURITY_ROOT}/include/
      ${NRF_SECURITY_ROOT}/include/mbedtls
)

# Add custom PSA includes (if enabled)
if(COMPILE_PSA_APIS)
  target_include_directories(mbedcrypto_common
    INTERFACE
      ${NRF_SECURITY_ROOT}/include/psa
  )
endif()

 # Add regular includes
target_include_directories(mbedcrypto_common
  INTERFACE
    ${ARM_MBEDTLS_PATH}/include
    ${ARM_MBEDTLS_PATH}/include/mbedtls
    ${ARM_MBEDTLS_PATH}/include/psa
    ${ARM_MBEDTLS_PATH}/library
)

append_with_prefix(src_crypto_core_builtin ${ARM_MBEDTLS_PATH}/library
  platform.c
  platform_util.c
)

if (COMPILE_PSA_APIS)
  # Add files for PSA build
  append_with_prefix(src_crypto_core_builtin ${ARM_MBEDTLS_PATH}/library/
    psa_crypto.c
    psa_crypto_aead.c
    psa_crypto_cipher.c
    psa_crypto_client.c
    psa_crypto_ecp.c
    psa_crypto_hash.c
    psa_crypto_mac.c
    psa_crypto_rsa.c
    psa_crypto_slot_management.c
    psa_crypto_storage.c
   )
endif()

target_sources(${mbedcrypto_target}
  PRIVATE
    ${src_crypto_core_builtin}
)

