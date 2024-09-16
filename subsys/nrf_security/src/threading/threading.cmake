#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file includes threading support required by the PSA crypto core
# Which was added in Mbed TLS 3.6.0.

if(CONFIG_MBEDTLS_THREADING_C AND NOT (CONFIG_PSA_CRYPTO_DRIVER_CC3XX OR CONFIG_CC3XX_BACKEND))

  append_with_prefix(src_crypto_base ${CMAKE_CURRENT_LIST_DIR}
    threading_alt.c
  )

  # Add include of threading_alt.h in library build
  target_include_directories(psa_crypto_library_config
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/include
  )

  # Add include of threading_alt.h in interface build
  target_include_directories(psa_crypto_config    
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/include
  )

endif()
