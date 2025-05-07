#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This resolves include-folders from binaries for legacy API usage
# This is not used inside TF-M which doesn't use legacy APIs

add_library(mbedcrypto_legacy_includes INTERFACE)

if(TARGET nrf_cc3xx_platform)
  target_include_directories(mbedcrypto_legacy_includes
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_platform,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

if(TARGET nrf_cc3xx_core_imported)
  target_include_directories(mbedcrypto_legacy_includes
    INTERFACE
      $<TARGET_PROPERTY:nrf_cc3xx_core_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

if(TARGET mbedcrypto_oberon_imported)
  target_include_directories(mbedcrypto_legacy_includes
    INTERFACE
      $<TARGET_PROPERTY:mbedcrypto_oberon_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()

if(TARGET mbedcrypto_oberon_mbedtls_imported)
  target_include_directories(mbedcrypto_legacy_includes
    INTERFACE
      $<TARGET_PROPERTY:mbedcrypto_oberon_mbedtls_imported,INTERFACE_INCLUDE_DIRECTORIES>
  )
endif()
