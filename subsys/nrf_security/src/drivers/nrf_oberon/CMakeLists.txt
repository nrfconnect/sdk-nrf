#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Build Oberon PSA crypto driver 
set(drivers_path ${OBERON_PSA_CORE_PATH}/oberon/drivers)

target_include_directories(psa_crypto_library_config
  INTERFACE
    ${drivers_path}
)

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

add_library(oberon_psa_driver STATIC
  ${src_crypto_oberon}
)

# Turn off warnings that Oberon are systematically
# triggering. Oberon is testing this deliverable before we get it so
# we don't need to re-test it with gcc warnings.
target_compile_options(oberon_psa_driver
  PRIVATE
    -Wno-uninitialized
    -Wno-maybe-uninitialized
    -Wno-unused-variable
    -Wno-unused-function
)

target_include_directories(oberon_psa_driver
  PRIVATE
    $<TARGET_PROPERTY:mbedcrypto_oberon_imported,INTERFACE_INCLUDE_DIRECTORIES>
)

# Link to the PSA interface and library PSA crypto configurations
target_link_libraries(oberon_psa_driver
  PRIVATE
    psa_crypto_library_config
    psa_interface
)


# Link to imported libraries for Oberon APIs.
target_link_libraries(oberon_psa_driver
  PRIVATE
    mbedcrypto_oberon_mbedtls_imported
    mbedcrypto_oberon_imported
)

# Link oberon_psa_core with this driver
target_link_libraries(oberon_psa_core
  PRIVATE
    oberon_psa_driver
)

target_link_libraries(mbedcrypto_oberon_mbedtls_imported
  INTERFACE
    psa_crypto_config_chosen
    ${mbedcrypto_target}
)

nrf_security_add_zephyr_options_library(oberon_psa_driver)
