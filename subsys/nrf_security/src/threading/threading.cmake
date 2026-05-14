#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MBEDTLS_THREADING_C)

  append_with_prefix(src_crypto_core_oberon ${CMAKE_CURRENT_LIST_DIR}
    threading_alt.c
  )

  # Add include of threading_alt.h in interface build
  target_include_directories(psa_crypto_config
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/include
  )

  # Add include of threading_alt.h in library build
  target_include_directories(psa_crypto_library_config
    INTERFACE
      ${CMAKE_CURRENT_LIST_DIR}/include
  )

endif()
