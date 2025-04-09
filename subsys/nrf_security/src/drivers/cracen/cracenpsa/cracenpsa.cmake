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

  # Note: We always need to have blkcipher.c and ctr_drbg.c since it
  # is used directly by many Cracen drivers.
  ${CMAKE_CURRENT_LIST_DIR}/src/blkcipher.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ctr_drbg.c
  ${CMAKE_CURRENT_LIST_DIR}/src/prng_pool.c
)

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
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_ASYMMETRIC_SIGNATURE_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/sign.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecdsa.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/hmac.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_HASH_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/hash.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_MAC_DRIVER)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/mac.c
  )
endif()

if(CONFIG_PSA_NEED_CRACEN_KEY_MANAGEMENT_DRIVER OR CONFIG_PSA_NEED_CRACEN_KMU_DRIVER OR CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
  list(APPEND cracen_driver_sources
    ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/key_management.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ed25519.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecdsa.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ecc.c
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
  zephyr_linker_sources(ROM_START SORT_KEY 0x1keys ${CMAKE_CURRENT_LIST_DIR}/src/platform_keys/platform_keys.ld)
endif()
