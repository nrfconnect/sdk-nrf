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
  list(APPEND src_crypto_oberon
       ${drivers_path}/oberon_helpers.c
  )

  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_KEY_PAIR_DRIVER     src_crypto_oberon ${drivers_path} oberon_key_pair.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_AEAD_DRIVER         src_crypto_oberon ${drivers_path} oberon_aead.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_KDF_DRIVER          src_crypto_oberon ${drivers_path} oberon_kdf.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_MAC_DRIVER          src_crypto_oberon ${drivers_path} oberon_mac.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_CIPHER_DRIVER       src_crypto_oberon ${drivers_path} oberon_cipher.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_RSA_DRIVER          src_crypto_oberon ${drivers_path} oberon_rsa.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_HASH_DRIVER         src_crypto_oberon ${drivers_path} oberon_hash.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_ECDSA_DRIVER        src_crypto_oberon ${drivers_path} oberon_ecdsa.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_ECDH_DRIVER         src_crypto_oberon ${drivers_path} oberon_ecdh.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_JPAKE_DRIVER        src_crypto_oberon ${drivers_path} oberon_jpake.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_SPAKE2P_DRIVER      src_crypto_oberon ${drivers_path} oberon_spake2p.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_SRP_DRIVER          src_crypto_oberon ${drivers_path} oberon_srp.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_CTR_DRBG_DRIVER     src_crypto_oberon ${drivers_path} oberon_ctr_drbg.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_HMAC_DRBG_DRIVER    src_crypto_oberon ${drivers_path} oberon_hmac_drbg.c)

  target_sources(${mbedcrypto_target} PRIVATE ${src_crypto_oberon})
endif()
