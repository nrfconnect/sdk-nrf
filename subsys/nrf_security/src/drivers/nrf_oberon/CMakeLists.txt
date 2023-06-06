#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(drivers_path ${OBERON_PSA_PATH}/drivers)

# Link to libraries
if(CONFIG_MBEDTLS_PSA_CRYPTO_C)
  target_link_libraries(${mbedcrypto_target} PRIVATE
    mbedcrypto_oberon_mbedtls_imported
    mbedcrypto_oberon_imported
  )

   target_include_directories(mbedcrypto_common
     INTERFACE
      # Add nrf_oberon includes
      ${NRFXLIB_DIR}/crypto/nrf_oberon/include/
      ${NRFXLIB_DIR}/crypto/nrf_oberon/include/mbedtls

      # Add Oberon PSA Crypto Driver includes
      ${drivers_path}
  )
endif()

if (COMPILE_PSA_APIS)
  list(APPEND                                                                     src_crypto_oberon ${drivers_path}/oberon_helpers.c)

  # The mapping from CONFIG_ to source file comes from the header file
  # includes in psa_crypto_driver_wrappers.c

  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_ACCEL_KEY_TYPES_OBERON    src_crypto_oberon ${drivers_path} oberon_key_pair.c)

  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_AEAD_SUPPORT_OBERON       src_crypto_oberon ${drivers_path} oberon_aead.c)
  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_KDF_SUPPORT_OBERON        src_crypto_oberon ${drivers_path} oberon_kdf.c)
  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_MAC_SUPPORT_OBERON        src_crypto_oberon ${drivers_path} oberon_mac.c)
  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_CIPHER_SUPPORT_OBERON     src_crypto_oberon ${drivers_path} oberon_cipher.c)
  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_RSA_SUPPORT_OBERON        src_crypto_oberon ${drivers_path} oberon_rsa.c)
  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_HASH_SUPPORT_OBERON       src_crypto_oberon ${drivers_path} oberon_hash.c)

  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_HAS_ASYM_SIGN_SUPPORT_OBERON  src_crypto_oberon ${drivers_path} oberon_ecdsa.c)

  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_ALG_ECDH_OBERON               src_crypto_oberon ${drivers_path} oberon_ecdh.c)

  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_ALG_CTR_DRBG_OBERON           src_crypto_oberon ${drivers_path} oberon_ctr_drbg.c)
  append_with_prefix_ifdef(CONFIG_PSA_CRYPTO_DRIVER_ALG_HMAC_DRBG_OBERON          src_crypto_oberon ${drivers_path} oberon_hmac_drbg.c)

  target_sources(${mbedcrypto_target} PRIVATE ${src_crypto_oberon})
endif()
