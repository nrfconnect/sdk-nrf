#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This directory contains C sources for the CRACEN PSA crypto driver

list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/include
)

list(APPEND cracen_driver_internal_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/src
)

list(APPEND cracen_driver_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa.c
  ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_keygen.c
  ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_key_management.c
)

if(BUILD_INSIDE_TFM)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_builtin_key_policy.c
  )
endif()

# Include hardware cipher implementation for all devices except nRF54LM20A,
# which does not support multipart cipher operations.
# nRF54LM20A uses only cracen_sw
if(NOT CONFIG_PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/aes/cracen_aes_ecb.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/aes/cracen_aes_cbc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_cipher.c
  )
endif()

if(NOT CONFIG_PSA_CRYPTO_DRIVER_ALG_PRNG_TEST)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_ctr_drbg.c
  )
endif()

if(CONFIG_CRACEN_IKG)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ikg/cracen_ikg_operations.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_AEAD_DRIVER AND NOT CONFIG_PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_aead.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_asymmetric.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_common.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_encryption_rsaes_pkcs1v15.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_encryption_rsaes_oaep.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_mgf1xor.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_sign_verify.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecdsa.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_keygen.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_eddsa_ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_eddsa_ed448.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_signature.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_signature.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_common.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_signature_rsapss.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_signature_rsassa_pkcs1v15.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_mgf1xor.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_HASH_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_hash.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_MAC_DRIVER AND NOT CONFIG_PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_mac.c
  )

  if(CONFIG_PSA_NEED_CRACEN_CMAC)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/mac/cracen_mac_cmac.c
    )
  endif()
endif()

if(CONFIG_PSA_NEED_CRACEN_HMAC)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/mac/cracen_mac_hmac.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER OR CONFIG_PSA_NEED_CRACEN_KMU_DRIVER OR CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_ikg.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/pake/cracen_wpa3_key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/pake/cracen_spake2p_key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/pake/cracen_srp_key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecdsa.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_keygen.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_ecc_key_management.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_common.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_keygen.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/rsa/cracen_rsa_coprime_check.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_PAKE_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_pake.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_spake2p.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_jpake.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_SRP_6)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_srp.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_WPA3_SAE)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_wpa3_sae.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KMU_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_kmu.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_eddsa_ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_eddsa_ed448.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecc/cracen_montgomery.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER OR CONFIG_PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecdh/cracen_ecdh_montgomery.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/ecdh/cracen_ecdh_weierstrass.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_kdf_common.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_key_derivation.c
  )

  if(CONFIG_PSA_NEED_CRACEN_HKDF)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_hkdf.c
    )
  endif()

  if(CONFIG_PSA_NEED_CRACEN_PBKDF2_HMAC)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_pbkdf2_hmac.c
    )
  endif()

  if(CONFIG_PSA_NEED_CRACEN_TLS12_ECJPAKE_TO_PMS)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_tls12_ecjpake_to_pms.c
    )
  endif()

  if(CONFIG_PSA_NEED_CRACEN_TLS12_PRF OR PSA_NEED_CRACEN_TLS12_PSK_TO_MS)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_tls12.c
    )
  endif()

  if(CONFIG_PSA_NEED_CRACEN_SRP_PASSWORD_HASH)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_srp_password_hash.c
    )
  endif()

  if(CONFIG_PSA_NEED_CRACEN_SP800_108_COUNTER_CMAC OR CONFIG_PSA_NEED_CRACEN_SP800_108_COUNTER_HMAC)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_sp800_108_ctr_mac.c
    )
  endif()

  if(CONFIG_PSA_NEED_CRACEN_WPA3_SAE_H2E)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_derivation/cracen_wpa3_sae_h2e.c
    )
  endif()
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_WRAP_DRIVER)
  # nRF54LM20A supports singlepart AES-ECB required by key wrap
  if(CONFIG_PSA_NEED_CRACEN_MULTIPART_WORKAROUNDS)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/internal/aes/cracen_aes_ecb.c
    )
  endif()

  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_wrap/cracen_key_wrap_common.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_wrap/cracen_key_wrap_kw.c
    ${CMAKE_CURRENT_LIST_DIR}/src/internal/key_wrap/cracen_key_wrap_kwp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_psa_key_wrap.c
  )
endif()
