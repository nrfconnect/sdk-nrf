#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This directory contains C sources for the CRACEN PSA crypto driver

list(APPEND cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/include
)

list(APPEND cracen_driver_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/cracen.c
  ${CMAKE_CURRENT_LIST_DIR}/src/common.c
  ${CMAKE_CURRENT_LIST_DIR}/src/mem_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ec_helpers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ecc.c
  ${CMAKE_CURRENT_LIST_DIR}/src/rndinrange.c

  # Note: We always need to have a version of cipher.c as it
  # is used directly by many Cracen drivers.
  ${CMAKE_CURRENT_LIST_DIR}/src/prng_pool.c
)

# Include hardware cipher implementation for all devices except nRF54LM20A
# nRF54LM20A uses only cracen_sw
if(NOT CONFIG_CRACEN_NEED_MULTIPART_WORKAROUNDS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cipher.c
  )
endif()

if(NOT CONFIG_PSA_CRYPTO_DRIVER_ALG_PRNG_TEST)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/ctr_drbg.c
  )
endif()

if(CONFIG_CRACEN_IKG)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/ikg_signature.c
  )
endif()

if(CONFIG_CRACEN_LIB_KMU)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/lib_kmu.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_AEAD_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/aead.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_ENCRYPTION_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/asymmetric.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rsaes_pkcs1v15.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rsaes_oaep.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rsamgf1xor.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/sign.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecdsa.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hmac.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rndinrange.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_ANY_RSA)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/rsapss.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rsassa_pkcs1v15.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rsamgf1xor.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_HASH_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/hash.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_MAC_DRIVER AND NOT CONFIG_CRACEN_NEED_MULTIPART_WORKAROUNDS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/mac.c
  )

  if(CONFIG_PSA_NEED_CRACEN_CMAC)
    list(APPEND cracen_driver_sources
      ${CMAKE_CURRENT_LIST_DIR}/src/cracen_mac_cmac.c
    )
  endif()
endif()

if(CONFIG_PSA_NEED_CRACEN_HMAC)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/cracen_mac_hmac.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hmac.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER OR CONFIG_PSA_NEED_CRACEN_KMU_DRIVER OR CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecdsa.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/rndinrange.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/rsa_keygen.c
    ${CMAKE_CURRENT_LIST_DIR}/src/coprime_check.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_PAKE_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/pake.c
    ${CMAKE_CURRENT_LIST_DIR}/src/spake2p.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ECJPAKE_SECP_R1_256)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/jpake.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_SRP_6)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/srp.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KMU_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/kmu.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/montgomery.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_AGREEMENT_DRIVER OR CONFIG_PSA_NEED_CRACEN_KEY_DERIVATION_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/key_derivation.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_PLATFORM_KEYS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/platform_keys/platform_keys.c
  )
endif()
