#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(drivers_path ${OBERON_PSA_PATH}/oberon/drivers)

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

if (COMPILE_PSA_APIS OR CONFIG_NRF_SECURITY_LEGACY_AND_PSA)

list(APPEND src_crypto_oberon
  ${drivers_path}/oberon_helpers.c
  ${drivers_path}/oberon_ecdh.c
  ${drivers_path}/oberon_ecdsa.c
  ${drivers_path}/oberon_ec_keys.c
  ${drivers_path}/oberon_jpake.c
  ${drivers_path}/oberon_spake2p.c
  ${drivers_path}/oberon_srp.c
  ${drivers_path}/oberon_rsa.c
)

  # This is a workaroudn to avoid a build warning since oberon_rsa.c has an unused variable when RSA is not enabled.
  if(CONFIG_PSA_NEED_OBERON_RSA_ANY_CRYPT OR CONFIG_PSA_NEED_OBERON_RSA_ANY_SIGN OR CONFIG_PSA_NEED_OBERON_KEY_MANAGEMENT_RSA)
    list(APPEND src_crypto_oberon
        ${drivers_path}/oberon_rsa.c
    )
  endif()

  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_KEY_MANAGEMENT_DRIVER        src_crypto_oberon ${drivers_path} oberon_key_management.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_AEAD_DRIVER                  src_crypto_oberon ${drivers_path} oberon_aead.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_KEY_DERIVATION_DRIVER        src_crypto_oberon ${drivers_path} oberon_key_derivation.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_MAC_DRIVER                   src_crypto_oberon ${drivers_path} oberon_mac.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_CIPHER_DRIVER                src_crypto_oberon ${drivers_path} oberon_cipher.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_HASH_DRIVER                  src_crypto_oberon ${drivers_path} oberon_hash.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_KEY_AGREEMENT_DRIVER         src_crypto_oberon ${drivers_path} oberon_key_agreement.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_PAKE_DRIVER                  src_crypto_oberon ${drivers_path} oberon_pake.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_ASYMMETRIC_SIGNATURE_DRIVER  src_crypto_oberon ${drivers_path} oberon_asymmetric_signature.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_ASYMMETRIC_ENCRYPTION_DRIVER src_crypto_oberon ${drivers_path} oberon_asymmetric_encrypt.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_CTR_DRBG_DRIVER              src_crypto_oberon ${drivers_path} oberon_ctr_drbg.c)
  append_with_prefix_ifdef(CONFIG_PSA_NEED_OBERON_HMAC_DRBG_DRIVER             src_crypto_oberon ${drivers_path} oberon_hmac_drbg.c)


elseif(CONFIG_NRF_SECURITY_LEGACY_AND_PSA)
  # When legacy and PSA are enabled, we need to include all the oberon drivers
  # since we don't have PSA_NEED_* symbols in Kconfig.
  list(APPEND src_crypto_oberon
       ${drivers_path}/oberon_helpers.c
       ${drivers_path}/oberon_ecdh.c
       ${drivers_path}/oberon_ecdsa.c
       ${drivers_path}/oberon_ec_keys.c
       ${drivers_path}/oberon_jpake.c
       ${drivers_path}/oberon_spake2p.c
       ${drivers_path}/oberon_srp.c
       ${drivers_path}/oberon_rsa.c
       ${drivers_path}/oberon_key_management.c
       ${drivers_path}/oberon_aead.c
       ${drivers_path}/oberon_key_derivation.c
       ${drivers_path}/oberon_mac.c
       ${drivers_path}/oberon_cipher.c
       ${drivers_path}/oberon_hash.c
       ${drivers_path}/oberon_key_agreement.c
       ${drivers_path}/oberon_pake.c
       ${drivers_path}/oberon_asymmetric_signature.c
       ${drivers_path}/oberon_asymmetric_encrypt.c
       ${drivers_path}/oberon_ctr_drbg.c
       ${drivers_path}/oberon_hmac_drbg.c)


endif()

  target_sources(${mbedcrypto_target} PRIVATE ${src_crypto_oberon})

  # Turn off warnings that Oberon are systematically
  # triggering. Oberon is testing this deliverable before we get it so
  # we don't need to re-test it with gcc warnings.
  target_compile_options(
	${mbedcrypto_target}
	PRIVATE
	-Wno-uninitialized
	-Wno-maybe-uninitialized
  )

endif()
