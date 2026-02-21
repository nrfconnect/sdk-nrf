#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(nrf_security_utils STATIC
  ${CMAKE_CURRENT_LIST_DIR}/nrf_security_mutexes.c
  ${CMAKE_CURRENT_LIST_DIR}/nrf_security_events.c
  ${CMAKE_CURRENT_LIST_DIR}/nrf_security_core.c
)

target_include_directories(psa_crypto_config
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_include_directories(psa_crypto_library_config
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

if(BUILD_INSIDE_TFM)
  # This gives access to cmsis, nrfx and mdk and the tfm_sp_log used by 
  # __assert.h
  target_link_libraries(nrf_security_utils
    PUBLIC
      platform_s
      tfm_sp_log
      tfm_psa_rot_partition_crypto
  )
else()
  # Do not link to the kernel target by name to avoid a cyclic dependency
  # (kernel -> zephyr_interface -> psa_crypto_config_chosen -> mbedcrypto_base
  #  -> nrf_security_utils -> kernel).
  # Using $<TARGET_FILE:kernel> resolves to the library path at generation time
  # without creating a link-level dependency edge, which breaks the cycle while
  # still giving us the kernel symbols (k_mutex_*, k_event_*, etc.).
  target_link_libraries(nrf_security_utils
    PRIVATE
      $<TARGET_FILE:kernel>
  )
  add_dependencies(nrf_security_utils kernel)
endif()

nrf_security_add_zephyr_options_library(nrf_security_utils)
