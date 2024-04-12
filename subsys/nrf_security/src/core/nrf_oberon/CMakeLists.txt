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
if(CONFIG_BUILD_WITH_TFM)
  # It is not supported to have a PSA core in the non-secure
  # image. PSA should be provided by TF-M, not an Oberon core inside
  # the non-secure image.
  #
  # But it is supported to have legacy mbedtls in the non-secure
  # image. We therefore need to add the legacy mbedtls header files to
  # the build. The TF-M PSA header files will be added elsewhere.
target_include_directories(mbedcrypto_common
  INTERFACE
    # Nordic PSA headers
    ${NRF_SECURITY_ROOT}/include
    # Mbed TLS (mbedcrypto) PSA headers
    ${ARM_MBEDTLS_PATH}/library
    ${ARM_MBEDTLS_PATH}/include
    ${ARM_MBEDTLS_PATH}/include/library
)
else()
target_include_directories(mbedcrypto_common
  INTERFACE
    # Nordic PSA headers
    ${NRF_SECURITY_ROOT}/include
    # Oberon PSA headers
    ${OBERON_PSA_PATH}/include
    ${OBERON_PSA_PATH}/library
    # Mbed TLS (mbedcrypto) PSA headers
    ${ARM_MBEDTLS_PATH}/include
    ${ARM_MBEDTLS_PATH}/library
)
endif()

append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_PATH}/library/
  platform.c
  platform_util.c
)

if (COMPILE_PSA_APIS)
  append_with_prefix(src_crypto_core_oberon ${OBERON_PSA_PATH}/library/
    psa_crypto.c
    psa_crypto_client.c
    psa_crypto_slot_management.c
    psa_crypto_storage.c
  )
endif()

target_sources(${mbedcrypto_target}
  PRIVATE
    ${src_crypto_core_oberon}
)
