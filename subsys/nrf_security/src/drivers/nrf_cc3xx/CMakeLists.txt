#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Get the include-folders from the imported CC3XX library
get_target_property(NRF_CC3XX_MBEDCRYPTO_INCLUDE_PATH 
  nrf_cc3xx_core_imported INTERFACE_INCLUDE_DIRECTORIES)

# Add nrf_cc3xx PSA Crypto Driver includes to psa_crypto_library_config
target_include_directories(psa_crypto_library_config
  INTERFACE
    ${NRF_CC3XX_MBEDCRYPTO_INCLUDE_PATH}
)

# Link oberon_psa_core with the imported libraries
target_link_libraries(oberon_psa_core
  INTERFACE
    nrf_cc3xx_psa_crypto_imported
    nrf_cc3xx_core_imported
    c
)