#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

append_with_prefix(src_crypto_base ${CMAKE_CURRENT_LIST_DIR}
  nrf_security_mutexes.c
  nrf_security_events.c
)

target_include_directories(psa_crypto_library_config
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_include_directories(psa_crypto_config
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
