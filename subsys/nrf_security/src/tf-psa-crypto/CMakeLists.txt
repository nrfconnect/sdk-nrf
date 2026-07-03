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

# Build TF-PSA-Crypto files when not building the TF-M S image.
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
    ecp.c
    ecp_curves.c
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

  # Add some TF-PSA-Crypto source files only when building client-only images
  # (TF-M NS, PSA_SSF_CRYPTO_CLIENT).
  # In other cases they are built as part of the PSA Crypto core, but when
  # building client-only images the functions these files implement are not
  # made available so we need their implementation in this image.
  if(NOT CONFIG_MBEDTLS_PSA_CRYPTO_C)
    append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/platform
      platform_util.c
    )
    append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/utilities
      constant_time.c
    )
  endif()

  if(CONFIG_MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS)
    append_with_prefix(src_crypto ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/legacy_sub/src
      aes.c
      cipher.c
      cipher_wrap.c
      ccm.c
      gcm.c
      md5.c
      rsa.c
      rsa_alt_helpers.c
      sha1.c
      sha256.c
      sha512.c
    )
  endif()

  target_include_directories(${tfpsacrypto_target}
    PUBLIC # public because needed by Mbed TLS which links to mbedcrypto via tfpsacrypto ALIAS below
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/utilities
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/legacy_sub/include
      ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/legacy_sub/src
  )
endif()

if(CONFIG_MBEDTLS)
  if(NOT (CONFIG_MBEDTLS_BUILTIN AND CONFIG_TF_PSA_CRYPTO_CUSTOM))
    message(FATAL_ERROR "Invalid Mbed TLS/TF-PSA-Crypto configuration.")
  endif()
  # Mbed TLS has a hard dependency on there being a tfpsacrypto library.
  # Making it as an alias to mbedcrypto works well.
  add_library(tfpsacrypto ALIAS ${tfpsacrypto_target})
endif()

target_sources(${tfpsacrypto_target} PRIVATE ${src_crypto})
