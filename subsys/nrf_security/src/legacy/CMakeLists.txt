#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
  return()
endif()

# Legacy APIs missing driver support or APIs assumed to be
# deprecated with more PSA API awareness.
append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
  dhm.c
  cipher.c
  cipher_wrap.c
  md.c
  pk.c
  pk_ecc.c
  pk_wrap.c
  pkwrite.c
  sha1.c
  sha256.c
  sha512.c
  sha3.c
  md5.c
  aesni.c
  aesce.c
  psa_util.c
  platform_util.c
)

# Add all files that are neccessary for (nrf_oberon or cracen) + builtin in legacy mode
# These files are used only as a convenience until PSA Crypto APIs are standarized
if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C OR
  CONFIG_PSA_CRYPTO_DRIVER_OBERON OR
  CONFIG_PSA_CRYPTO_DRIVER_CRACEN)
  append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
    aes.c
    cmac.c
    ccm.c
    chachapoly.c
    ecdh.c
    ecdsa.c
    ecp.c
    ecp_curves.c
    ecp_curves_new.c
    gcm.c
    hkdf.c
    hmac_drbg.c
    rsa.c
    rsa_alt_helpers.c
  )
endif()

if(CONFIG_HAS_HW_NRF_CC310)
append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
  gcm.c
)
endif()

# Legacy files should only be added in certain configurations
append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
  ctr_drbg.c
  entropy.c
)

# Legacy Files needed when Oberon is disabled
if(NOT CONFIG_OBERON_BACKEND)
  append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
    chacha20.c
    poly1305.c
    ecjpake.c
    platform.c
  )
endif()

# Add oberon-specific replacement files
if (CONFIG_OBERON_ONLY_ENABLED)
  list(APPEND src_crypto_legacy
    ${CMAKE_CURRENT_LIST_DIR}/ecjpake_oberon.c
  )
else()
  list(APPEND src_crypto_legacy
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
  # Link to the nrf_oberon mbedcrypto library
  target_link_libraries(${mbedcrypto_target}
    PRIVATE
      mbedcrypto_oberon_mbedtls_imported
  )

  target_link_libraries(mbedcrypto_oberon_mbedtls_imported
    INTERFACE
      mbedcrypto_base
  )

  # Add nrf_oberon Mbed TLS includes to mbedcrypto library
  target_include_directories(${mbedcrypto_target}
    INTERFACE
      $<TARGET_PROPERTY:mbedcrypto_oberon_mbedtls_imported,INTERFACE_INCLUDE_DIRECTORIES>
	)
endif()

if(TARGET mbedcrypto_oberon_imported)
  # Add nrf_oberon includes to mbedcrypto library
  target_include_directories(${mbedcrypto_target}
    INTERFACE
      $<TARGET_PROPERTY:mbedcrypto_oberon_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C AND CONFIG_CC3XX_BACKEND)
  # Link legacy nrf_cc3xx library if it is enabled
  # This will only be used for secure-only builds
  target_link_libraries(${mbedcrypto_target} 
    PRIVATE
      nrf_cc3xx_legacy_crypto_imported
  )
endif()

if(TARGET nrf_cc3xx_core_imported)
  # nrf_cc3xx needs functionality from mbedcrypto_base
  target_link_libraries(nrf_cc3xx_core_imported
    INTERFACE
      mbedcrypto_base
  )
endif()

# Add the nrf_cc3xx core include folder to library build (if it exists)
if (TARGET nrf_cc3xx_core_imported)
  target_include_directories(psa_crypto_library_config
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_core_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

# Add the nrf_cc3xx platform include folder to library build (if it exists)
if (TARGET nrf_cc3xx_platform)
  target_include_directories(psa_crypto_library_config
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_platform,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()