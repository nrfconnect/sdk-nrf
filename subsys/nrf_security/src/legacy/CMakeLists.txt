#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Legacy APIs missing driver support or APIs assumed to be
# deprecated with more PSA API awareness.
append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
  dhm.c
  cipher.c
  cipher_wrap.c
  hash_info.c
  md.c
  pk.c
  pk_wrap.c
  pkwrite.c
  sha1.c
  sha256.c
  sha512.c
  ripemd160.c
  md5.c
  aesni.c
  aria.c
  camellia.c
  des.c
)

# Add all files that are neccessary for nrf_oberon + builtin in legacy mode
# These files are used only as a convenience until PSA Crypto APIs are standarized
if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C OR
  CONFIG_PSA_CRYPTO_DRIVER_BUILTIN OR
  CONFIG_PSA_CRYPTO_DRIVER_OBERON)
  append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
    cmac.c
    ccm.c
    chachapoly.c
    ecdh.c
    ecdsa.c
    ecp.c
    ecp_curves.c
    gcm.c
    hkdf.c
    hmac_drbg.c
    rsa.c
    rsa_alt_helpers.c
  )
endif()

if(CONFIG_MBEDTLS_PSA_CRYPTO_SPM AND CONFIG_MBEDTLS_HKDF_C)
  # TF-M 1.7.0 requires legacy mbedtls_hkdf function in key loader.
  append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
    hkdf.c
  )
endif()

if(CONFIG_CRYPTOCELL_CC310_USABLE)
append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
  gcm.c
)
endif()

# Legacy files should only be added in certain configurations
if(NOT (MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG OR CONFIG_MBEDTLS_PSA_CRYPTO_SPM))
  append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
    ctr_drbg.c
    entropy.c
  )
endif()

# Legacy Files needed when Oberon is disabled
if(NOT CONFIG_OBERON_BACKEND)
  append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
    chacha20.c
    poly1305.c
    ecjpake.c
  )
endif()

# Add oberon-specific replacement files
if (CONFIG_OBERON_ONLY_ENABLED)
  list(APPEND src_crypto_legacy
    ${CMAKE_CURRENT_LIST_DIR}/aes.c
    ${CMAKE_CURRENT_LIST_DIR}/aes_oberon.c
    ${CMAKE_CURRENT_LIST_DIR}/ecjpake_oberon.c
  )
else()
  list(APPEND src_crypto_legacy
    ${ARM_MBEDTLS_PATH}/library/aes.c
    ${CMAKE_CURRENT_LIST_DIR}/dhm_ext.c
  )
endif()

# Add legacy files to mbedcrypto_target
if(DEFINED src_crypto_legacy)
  target_sources(${mbedcrypto_target}
    PRIVATE
      ${src_crypto_legacy}
  )
endif()

if(DEFINED src_crypto_legacy_base)
  target_sources(mbedcrypto_base
    PRIVATE
      ${src_crypto_legacy_base}
  )
endif()

if(TARGET mbedcrypto_oberon_mbedtls_imported)

  # Get the include paths from nrf_oberon libraries
  get_target_property(NRF_OBERON_MBEDTLS_INCLUDE_PATH mbedcrypto_oberon_mbedtls_imported
    INTERFACE_INCLUDE_DIRECTORIES
  )
  get_target_property(NRF_OBERON_INCLUDE_PATH mbedcrypto_oberon_imported
    INTERFACE_INCLUDE_DIRECTORIES
  )

  # Link to the nrf_oberon mbedcrypto library
  target_link_libraries(${mbedcrypto_target}
    PRIVATE
      mbedcrypto_oberon_mbedtls_imported
  )

target_link_libraries(mbedcrypto_oberon_mbedtls_imported
  INTERFACE
    mbedcrypto_base
)

  # Add nrf_oberon includes to mbedcrypto library
  target_include_directories(${mbedcrypto_target}
    INTERFACE
      ${NRF_OBERON_MBEDTLS_INCLUDE_PATH}
      ${NRF_OBERON_INCLUDE_PATH}
  )
endif()


if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C AND CONFIG_CC3XX_BACKEND)
  # Link directly with cc3xx platform/mbedcrypto libraries
  target_link_libraries(${mbedcrypto_target} PRIVATE
    nrf_cc3xx_legacy_crypto_imported
  )

  target_link_libraries(nrf_cc3xx_core_imported
    INTERFACE
      mbedcrypto_base
  )

  # Add cc3xx platform/mbedcrypto includes to mbedcrypto library
  target_include_directories(mbedcrypto_common
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_core_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

if(TARGET nrf_cc3xx_core_imported)
  # The CC3XX needs funcionality which is provided by the inluded source files in src_crypto_legacy
  # Link CC3XX with mbedcrypto_target because of link order dependency
  target_link_libraries(nrf_cc3xx_core_imported INTERFACE
    ${mbedcrypto_target}
  )

  get_target_property(NRF_CC3XX_MBEDCRYPTO_INCLUDE_PATH nrf_cc3xx_core_imported INTERFACE_INCLUDE_DIRECTORIES)

  target_include_directories(mbedcrypto_common
    INTERFACE
      ${NRF_CC3XX_MBEDCRYPTO_INCLUDE_PATH}
  )
endif()

target_include_directories(${mbedcrypto_target}
INTERFACE
  $<$<TARGET_EXISTS:nrf_cc3xx_platform>:$<TARGET_PROPERTY:nrf_cc3xx_platform,INTERFACE_INCLUDE_DIRECTORIES>>
)
