#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Convert all standard Kconfig variables for mbed TLS (strip CONFIG_)
kconfig_check_and_set_base(MBEDTLS_PLATFORM_MEMORY)
kconfig_check_and_set_base(MBEDTLS_PLATFORM_C)
kconfig_check_and_set_base(MBEDTLS_MEMORY_C)
kconfig_check_and_set_base(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
kconfig_check_and_set_base(MBEDTLS_CIPHER_C)
kconfig_check_and_set_base(MBEDTLS_NO_PLATFORM_ENTROPY)
kconfig_check_and_set_base(MBEDTLS_ENTROPY_C)
kconfig_check_and_set_base(MBEDTLS_CTR_DRBG_C)
kconfig_check_and_set_base(MBEDTLS_CTR_DRBG_USE_128_BIT_KEY)
kconfig_check_and_set_base(MBEDTLS_HMAC_DRBG_C)
kconfig_check_and_set_base(MBEDTLS_ENTROPY_FORCE_SHA256)
kconfig_check_and_set_base(MBEDTLS_AES_C)
kconfig_check_and_set_base(MBEDTLS_AES_ALT)
kconfig_check_and_set_base(MBEDTLS_AES_SETKEY_ENC_ALT)
kconfig_check_and_set_base(MBEDTLS_AES_SETKEY_DEC_ALT)
kconfig_check_and_set_base(MBEDTLS_AES_ENCRYPT_ALT)
kconfig_check_and_set_base(MBEDTLS_AES_DECRYPT_ALT)
kconfig_check_and_set_base(MBEDTLS_CIPHER_MODE_CBC)
kconfig_check_and_set_base(MBEDTLS_CIPHER_PADDING_PKCS7)
kconfig_check_and_set_base(MBEDTLS_CIPHER_PADDING_ONE_AND_ZEROS)
kconfig_check_and_set_base(MBEDTLS_CIPHER_PADDING_ZEROS_AND_LEN)
kconfig_check_and_set_base(MBEDTLS_CIPHER_PADDING_ZEROS)

kconfig_check_and_set_base(MBEDTLS_AES_FEWER_TABLES)
kconfig_check_and_set_base(MBEDTLS_AES_ROM_TABLES)

kconfig_check_and_set_base(MBEDTLS_CIPHER_MODE_CTR)
kconfig_check_and_set_base(MBEDTLS_CIPHER_MODE_CFB)
kconfig_check_and_set_base(MBEDTLS_CIPHER_MODE_OFB)
kconfig_check_and_set_base(MBEDTLS_CIPHER_MODE_XTS)
kconfig_check_and_set_base(MBEDTLS_CMAC_C)
kconfig_check_and_set_base(MBEDTLS_CMAC_ALT)
kconfig_check_and_set_base(MBEDTLS_CCM_C)
kconfig_check_and_set_base(MBEDTLS_CCM_ALT)
kconfig_check_and_set_base(MBEDTLS_GCM_C)
kconfig_check_and_set_base(MBEDTLS_GCM_ALT)
kconfig_check_and_set_base(MBEDTLS_CHACHA20_ALT)
kconfig_check_and_set_base(MBEDTLS_CHACHA20_C)
kconfig_check_and_set_base(MBEDTLS_POLY1305_ALT)
kconfig_check_and_set_base(MBEDTLS_POLY1305_C)
kconfig_check_and_set_base(MBEDTLS_CHACHAPOLY_C)
kconfig_check_and_set_base(MBEDTLS_CHACHAPOLY_ALT)
kconfig_check_and_set_base(MBEDTLS_DHM_C)
kconfig_check_and_set_base(MBEDTLS_DHM_ALT)
kconfig_check_and_set_base(MBEDTLS_ECP_C)
kconfig_check_and_set_base(MBEDTLS_ECP_ALT)
kconfig_check_and_set_base(MBEDTLS_ECDH_C)
kconfig_check_and_set_base(MBEDTLS_ECDH_GEN_PUBLIC_ALT)
kconfig_check_and_set_base(MBEDTLS_ECDH_COMPUTE_SHARED_ALT)
kconfig_check_and_set_base(MBEDTLS_ECDSA_C)
kconfig_check_and_set_base(MBEDTLS_ECDSA_DETERMINISTIC)
kconfig_check_and_set_base(MBEDTLS_ECDSA_GENKEY_ALT)
kconfig_check_and_set_base(MBEDTLS_ECDSA_SIGN_ALT)
kconfig_check_and_set_base(MBEDTLS_ECDSA_VERIFY_ALT)
kconfig_check_and_set_base(MBEDTLS_ECJPAKE_C)
kconfig_check_and_set_base(MBEDTLS_ECJPAKE_ALT)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP192R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP224R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP256R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP384R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP521R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP192K1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP224K1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_SECP256K1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_BP256R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_BP384R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_BP512R1_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
kconfig_check_and_set_base(MBEDTLS_ECP_DP_CURVE448_ENABLED)
kconfig_check_and_set_base(MBEDTLS_RSA_C)
kconfig_check_and_set_base(MBEDTLS_RSA_ALT)
kconfig_check_and_set_base(MBEDTLS_GENPRIME)
kconfig_check_and_set_base(MBEDTLS_PKCS1_V15)
kconfig_check_and_set_base(MBEDTLS_PKCS1_V21)
kconfig_check_and_set_base(MBEDTLS_MD5_C)
kconfig_check_and_set_base(MBEDTLS_SHA1_ALT)
kconfig_check_and_set_base(MBEDTLS_SHA1_C)
kconfig_check_and_set_base(MBEDTLS_SHA224_ALT)
kconfig_check_and_set_base(MBEDTLS_SHA224_C)
kconfig_check_and_set_base(MBEDTLS_SHA256_ALT)
kconfig_check_and_set_base(MBEDTLS_SHA256_C)
kconfig_check_and_set_base(MBEDTLS_SHA384_C)
kconfig_check_and_set_base(MBEDTLS_SHA384_ALT)
kconfig_check_and_set_base(MBEDTLS_SHA512_C)
kconfig_check_and_set_base(MBEDTLS_SHA512_ALT)
kconfig_check_and_set_base(MBEDTLS_HKDF_C)
kconfig_check_and_set_base(MBEDTLS_MD_C)
kconfig_check_and_set_base(MBEDTLS_PK_C)
kconfig_check_and_set_base(MBEDTLS_PKCS5_C)
kconfig_check_and_set_base(MBEDTLS_PK_PARSE_C)
kconfig_check_and_set_base(MBEDTLS_PK_WRITE_C)
kconfig_check_and_set_base(MBEDTLS_DEBUG_C)
kconfig_check_and_set_base(MBEDTLS_MEMORY_DEBUG)

kconfig_check_and_set_base(MBEDTLS_PSA_CRYPTO_SPM)

kconfig_check_and_set_base(MBEDTLS_PSA_CRYPTO_C)

kconfig_check_and_set_base_to_one(MBEDTLS_PLATFORM_EXIT_ALT)
kconfig_check_and_set_base_to_one(MBEDTLS_PLATFORM_FPRINTF_ALT)
kconfig_check_and_set_base_to_one(MBEDTLS_PLATFORM_PRINTF_ALT)
kconfig_check_and_set_base_to_one(MBEDTLS_PLATFORM_SNPRINTF_ALT)
kconfig_check_and_set_base_to_one(MBEDTLS_PLATFORM_SETUP_TEARDOWN_ALT)
kconfig_check_and_set_base_to_one(MBEDTLS_ENTROPY_HARDWARE_ALT)
kconfig_check_and_set_base_to_one(MBEDTLS_THREADING_ALT)
kconfig_check_and_set_base_to_one(MBEDTLS_PLATFORM_ZEROIZE_ALT)

kconfig_check_and_set_base(MBEDTLS_X509_USE_C)
kconfig_check_and_set_base(MBEDTLS_X509_CHECK_KEY_USAGE)
kconfig_check_and_set_base(MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE)
kconfig_check_and_set_base(MBEDTLS_X509_CREATE_C)
kconfig_check_and_set_base(MBEDTLS_X509_CRL_PARSE_C)
kconfig_check_and_set_base(MBEDTLS_X509_CRT_PARSE_C)
kconfig_check_and_set_base(MBEDTLS_X509_CSR_PARSE_C)
kconfig_check_and_set_base(MBEDTLS_X509_CSR_WRITE_C)
kconfig_check_and_set_base(MBEDTLS_X509_REMOVE_INFO)

kconfig_check_and_set_base(MBEDTLS_SSL_CLI_C)
kconfig_check_and_set_base(MBEDTLS_SSL_SRV_C)
kconfig_check_and_set_base(MBEDTLS_SSL_TLS_C)
kconfig_check_and_set_base(MBEDTLS_SSL_PROTO_TLS1_2)
kconfig_check_and_set_base(MBEDTLS_SSL_COOKIE_C)
kconfig_check_and_set_base(MBEDTLS_SSL_PROTO_DTLS)
kconfig_check_and_set_base(MBEDTLS_SSL_DTLS_ANTI_REPLAY)
kconfig_check_and_set_base(MBEDTLS_SSL_DTLS_HELLO_VERIFY)
kconfig_check_and_set_base(MBEDTLS_SSL_DTLS_SRTP)
kconfig_check_and_set_base(MBEDTLS_SSL_DTLS_CLIENT_PORT_REUSE)
kconfig_check_and_set_base(MBEDTLS_SSL_DTLS_CONNECTION_ID)
kconfig_check_and_set_base(MBEDTLS_SSL_DTLS_BADMAC_LIMIT)

kconfig_check_and_set_base(MBEDTLS_SSL_ALL_ALERT_MESSAGES)
kconfig_check_and_set_base(MBEDTLS_SSL_CONTEXT_SERIALIZATION)
kconfig_check_and_set_base(MBEDTLS_SSL_DEBUG_ALL)
kconfig_check_and_set_base(MBEDTLS_SSL_KEEP_PEER_CERTIFICATE)
kconfig_check_and_set_base(MBEDTLS_SSL_RENEGOTIATION)
kconfig_check_and_set_base(MBEDTLS_SSL_SESSION_TICKETS)
kconfig_check_and_set_base(MBEDTLS_SSL_SERVER_NAME_INDICATION)
kconfig_check_and_set_base(MBEDTLS_SSL_CACHE_C)
kconfig_check_and_set_base(MBEDTLS_SSL_TICKET_C)
kconfig_check_and_set_base(MBEDTLS_SSL_EXPORT_KEYS)
kconfig_check_and_set_base(MBEDTLS_SSL_CIPHERSUITES)
kconfig_check_and_set_base(MBEDTLS_SSL_EXTENDED_MASTER_SECRET)

kconfig_check_and_set_base(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)

# Set integer values from Kconfig
kconfig_check_and_set_base_int(MBEDTLS_SSL_OUT_CONTENT_LEN)
kconfig_check_and_set_base_int(MBEDTLS_SSL_IN_CONTENT_LEN)
kconfig_check_and_set_base_int(MBEDTLS_ENTROPY_MAX_SOURCES)
kconfig_check_and_set_base_int(MBEDTLS_MPI_WINDOW_SIZE)
kconfig_check_and_set_base_int(MBEDTLS_MPI_MAX_SIZE)

# Set all enabled TLS/DTLS key exchange methods
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_DHE_PSK_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_RSA_PSK_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_RSA_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_DHE_RSA_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED)
kconfig_check_and_set_base(MBEDTLS_KEY_EXCHANGE_ECJPAKE_ENABLED)

#
# CC3XX flags for threading and platform zeroize
#
kconfig_check_and_set_base(MBEDTLS_THREADING_C)
kconfig_check_and_set_base(MBEDTLS_THREADING_ALT)
if (CONFIG_CC3XX_BACKEND)
  # TODO: Why don't we use kconfig_check_and_set_base for
  # MBEDTLS_PLATFORM_ZEROIZE_ALT ?
  set(MBEDTLS_PLATFORM_ZEROIZE_ALT TRUE)

  if(NOT DEFINED MBEDTLS_THREADING_C)

	# TODO: Why do we override these when CONFIG_CC3XX_BACKEND?
	# Shouldn't the Kconfig be corrected instead? Now we have Kconfig
	# and mbedtls configs saying conlficting things, which is not
	# great.
    set(MBEDTLS_THREADING_C TRUE)
    set(MBEDTLS_THREADING_ALT TRUE)
  endif()
endif()


# Convert defines required even in PSA mode
kconfig_check_and_set_base_depends(MBEDTLS_SHA1_C
  PSA_WANT_ALG_SHA_1
)

kconfig_check_and_set_base_depends(MBEDTLS_SHA256_C
  PSA_WANT_ALG_SHA_256
)

kconfig_check_and_set_base_depends(MBEDTLS_SHA512_C
  PSA_WANT_ALG_SHA_512
)

kconfig_check_and_set_base_depends(MBEDTLS_ECP_C
  PSA_WANT_ALG_ECDH
)

kconfig_check_and_set_base_depends(MBEDTLS_ECDH_C
  PSA_WANT_ALG_ECDH
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECDSA_C
  PSA_WANT_ALG_ECDSA
)

kconfig_check_and_set_base_depends(MBEDTLS_ECP_C
  PSA_WANT_ALG_ECDSA
)

kconfig_check_and_set_base_depends(MBEDTLS_ECDSA_DETERMINISTIC
  PSA_WANT_ALG_DETERMINISTIC_ECDSA
  PSA_WANT_ALG_HMAC_DRBG
)

kconfig_check_and_set_base_depends(MBEDTLS_HMAC_DRBG_C
  PSA_WANT_ALG_HMAC_DRBG
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP192R1_ENABLED
  PSA_WANT_ECC_SECP_R1_192
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP224R1_ENABLED
  PSA_WANT_ECC_SECP_R1_224
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP256R1_ENABLED
  PSA_WANT_ECC_SECP_R1_256
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP384R1_ENABLED
  PSA_WANT_ECC_SECP_R1_384
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP521R1_ENABLED
  PSA_WANT_ECC_SECP_R1_521
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP192K1_ENABLED
  PSA_WANT_ECC_SECP_K1_192
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP224K1_ENABLED
  PSA_WANT_ECC_SECP_K1_224
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_SECP256K1_ENABLED
  PSA_WANT_ECC_SECP_K1_256
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_BP256R1_ENABLED
  PSA_WANT_ECC_BRAINPOOL_P_R1_256
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_BP384R1_ENABLED
  PSA_WANT_ECC_BRAINPOOL_P_R1_384
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_BP512R1_ENABLED
  PSA_WANT_ECC_BRAINPOOL_P_R1_512
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_CURVE25519_ENABLED
  PSA_WANT_ECC_MONTGOMERY_255
)

Kconfig_check_and_set_base_depends(MBEDTLS_ECP_DP_CURVE448_ENABLED
  PSA_WANT_ECC_MONTGOMERY_448
)

# Ensure that MBEDTLS_SHA224_C is set if MBEDTLS_SHA256_C
# to prevent build errors.
kconfig_check_and_set_base_depends(MBEDTLS_SHA224_C
  MBEDTLS_SHA256_C
)

if(CONFIG_GENERATE_MBEDTLS_CFG_FILE)
  # Generate the mbed TLS config file (default nrf-config.h)
  configure_file(${NRF_SECURITY_ROOT}/configs/legacy_crypto_config.h.template
    ${generated_include_path}/${CONFIG_MBEDTLS_CFG_FILE}
  )

  # Copy an empty user-config to help with legacy build
  configure_file(${NRF_SECURITY_ROOT}/configs/psa_crypto_config.h.template
    ${generated_include_path}/${CONFIG_MBEDTLS_USER_CONFIG_FILE}
  )
endif()
