#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Note: Reusing the name oberon_psa_core as we don't have a normalized
#       name for "any" PSA core in the current design

add_library(oberon_psa_core STATIC
    psa_core_lite.c
    ${NRF_SECURITY_ROOT}/src/psa_crypto_driver_wrappers.c
)

target_link_libraries(oberon_psa_core
  PRIVATE
    psa_crypto_library_config
)

nrf_security_add_zephyr_options_library(oberon_psa_core)

target_link_libraries(${mbedcrypto_target}
  PRIVATE
    oberon_psa_core
)
