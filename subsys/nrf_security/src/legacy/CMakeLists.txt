#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(is_getting_includes FALSE)

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
  ripemd160.c
  md5.c
  aesni.c
  aesce.c
  aria.c
  camellia.c
  des.c
  psa_util.c
  platform_util.c
)

# Add all files that are neccessary for (nrf_oberon or cracen) + builtin in legacy mode
# These files are used only as a convenience until PSA Crypto APIs are standarized
if(CONFIG_MBEDTLS_LEGACY_CRYPTO_C OR
  CONFIG_PSA_CRYPTO_DRIVER_OBERON OR
  CONFIG_PSA_CRYPTO_DRIVER_CRACEN)
  append_with_prefix(src_crypto_legacy ${ARM_MBEDTLS_PATH}/library
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
if(NOT CONFIG_MBEDTLS_PSA_CRYPTO_SPM)
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
    platform.c
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
  set(is_getting_includes TRUE)

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
  set(is_getting_includes TRUE)

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
  set(is_getting_includes TRUE)

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

if(NOT is_getting_includes)
  if(NOT DEFINED ARM_MBEDTLS_PATH)
  message(FATAL_ERROR "misconfiguration")
  endif()

  # Add mbedtls legacy include paths to the mbedtls legacy library and
  # it's users (users get the headers because we use interface).  Also
  # since nrf_security only provides some of the headers, add the rest
  # from Zephyr. We place nrf_security before ARM_MBEDTLS_PATH so that
  # nrf_security takes precedence.
  target_include_directories(${mbedcrypto_target}
  INTERFACE
  ${NRF_SECURITY_ROOT}/include
  ${OBERON_PSA_PATH}/include
  ${OBERON_PSA_PATH}/library
  ${ARM_MBEDTLS_PATH}/include
  ${ARM_MBEDTLS_PATH}/library
  )
endif()
