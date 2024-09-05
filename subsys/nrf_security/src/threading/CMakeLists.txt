#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This file includes threading support required by the PSA crypto core
# Which was added in Mbed TLS 3.6.0.

# Return if threading is not enabled at all
if(NOT CONFIG_MBEDTLS_THREADING_C)
  return()
endif()

# If we are in a local build where the CC3XX driver or CC3XX backend is enabed
# then there is already a solution for mutex for the core
if(CONFIG_PSA_CRYPTO_DRIVER_CC3XX OR CONFIG_CC3XX_BACKEND)
  return()
endif()

# Add support for threading using 
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