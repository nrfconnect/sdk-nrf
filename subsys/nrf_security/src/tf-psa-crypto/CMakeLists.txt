#
# Copyright (c) 2026 - 2099 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MBEDTLS_MEMORY_BUFFER_ALLOC_C)
  append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/platform
    memory_buffer_alloc.c
  )
endif()

if(NOT CONFIG_MBEDTLS_PSA_CRYPTO_SPM)

  append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/utilities
    asn1parse.c
    asn1write.c
    base64.c
    oid.c
    pem.c
    pkcs5.c
  )

  append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/legacy_sub/src
    bignum.c
    bignum_core.c
    ccm.c
    cipher.c
    cipher_wrap.c
    ecp.c
    ecp_curves.c
    gcm.c
    psa_util_internal.c
  )

  append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/extras
    md.c
    nist_kw.c
    pk.c
    pk_ecc.c
    pk_rsa.c
    pk_wrap.c
    pkparse.c
    pkwrite.c
  )

  append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/core
    psa_util.c
  )

  target_include_directories(${mbedcrypto_target}
    PRIVATE
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/utilities
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/legacy_sub/include
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/legacy_sub/src
  )
endif()

target_sources(${mbedcrypto_target} PRIVATE ${src_crypto})
