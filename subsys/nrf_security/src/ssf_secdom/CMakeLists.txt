#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# NOTE that this is added as a duplication to ensure that SSF client gets
# access to the relevant include folders, without PSA core or PSA drivers
# being built.


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
    ${OBERON_PSA_PATH}/include
    ${OBERON_PSA_PATH}/library
    # Mbed TLS (mbedcrypto) PSA headers
    ${ARM_MBEDTLS_PATH}/include
    ${ARM_MBEDTLS_PATH}/library
)

target_sources(${mbedcrypto_target}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ssf_crypto.c
)
