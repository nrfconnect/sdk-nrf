#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add generated path first in list (order dependent)
target_include_directories(mbedcrypto_common
    INTERFACE
      ${generated_include_path}
)

# Add regular includes
# Note, the order of include matters
target_include_directories(mbedcrypto_common
  INTERFACE
    # Nordic PSA headers
    ${NRF_SECURITY_ROOT}/include
    # Oberon PSA headers
    ${OBERON_PSA_PATH}/core/include
    ${OBERON_PSA_PATH}/core/library
    # Mbed TLS (mbedcrypto) PSA headers
    ${ARM_MBEDTLS_PATH}/include
    ${ARM_MBEDTLS_PATH}/library
)

append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_PATH}/core/library/
  platform.c
  platform_util.c
)

if (COMPILE_PSA_APIS)
  append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_PATH}/core/library/
    psa_crypto.c
    psa_crypto_client.c
    psa_crypto_slot_management.c
    psa_crypto_storage.c
    psa_its_file.c
  )
endif()

target_sources(${mbedcrypto_target}
  PRIVATE
    ${src_crypto_core_oberon}
)
