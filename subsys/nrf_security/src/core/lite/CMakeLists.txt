#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(psa_core STATIC
  psa_core_lite.c
  ${NRF_SECURITY_ROOT}/src/psa_crypto_driver_wrappers.c
)

target_link_libraries(psa_core
  PRIVATE
    psa_crypto_library_config
)

nrf_security_add_zephyr_options_library(psa_core)

target_link_libraries(${mbedcrypto_target}
  PRIVATE
    psa_core
)
