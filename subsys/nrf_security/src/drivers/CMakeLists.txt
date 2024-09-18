#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nrf_cc3xx_platform is not a driver in the old APIs. Once ctr_drbg/hmac_drbg
# gets real driver API support then the placement here makes more sense
if(TARGET nrf_cc3xx_platform)
  add_subdirectory(nrf_cc3xx_platform)
endif()

if(CONFIG_MBEDTLS_PSA_CRYPTO_C)
  if(CONFIG_PSA_CRYPTO_DRIVER_CC3XX)
    add_subdirectory(nrf_cc3xx)
  endif()

  if(CONFIG_PSA_CRYPTO_DRIVER_CRACEN)
    add_subdirectory(cracen)
  endif()

  add_subdirectory(zephyr)

  if(CONFIG_PSA_CRYPTO_DRIVER_OBERON)
    add_subdirectory(nrf_oberon)
  endif()
endif()

# Add includes for any legacy Mbed TLS configurations


# Add the nrf_cc3xx platform include folder to library build (if it exists)
if(TARGET nrf_cc3xx_platform)
  target_include_directories(psa_crypto_config
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_platform,INTERFACE_INCLUDE_DIRECTORIES>
  )

  target_include_directories(psa_crypto_library_config
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_platform,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

# Add the nrf_cc3xx core include folder to interface/library build (if it exists)
if(TARGET nrf_cc3xx_core_imported)
  target_include_directories(psa_crypto_config
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_core_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )

  target_include_directories(psa_crypto_library_config
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_core_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

# Add the mbedcrypto_oberon_mbedtls_imported include folder to interface/library build (if it exists)
if(TARGET mbedcrypto_oberon_mbedtls_imported)
  target_include_directories(psa_crypto_config
    INTERFACE
      # Add nrf_oberon includes
      ${NRFXLIB_DIR}/crypto/nrf_oberon/include/
      ${NRFXLIB_DIR}/crypto/nrf_oberon/include/mbedtls
      $<TARGET_PROPERTY:mbedcrypto_oberon_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )

  # Possibly duplicate
  target_include_directories(psa_crypto_config
    INTERFACE
      $<TARGET_PROPERTY:mbedcrypto_oberon_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
  target_include_directories(psa_crypto_library_config
    INTERFACE
      $<TARGET_PROPERTY:mbedcrypto_oberon_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )

  # Add includes relevant for library build (not PSA crypto interface)
  target_include_directories(psa_crypto_library_config
    INTERFACE
      # Add nrf_oberon includes
      ${NRFXLIB_DIR}/crypto/nrf_oberon/include/
      ${NRFXLIB_DIR}/crypto/nrf_oberon/include/mbedtls
  )

  target_link_libraries(mbedcrypto_oberon_mbedtls_imported
    INTERFACE
      psa_crypto_config
      mbedcrypto_base
  )
endif()
