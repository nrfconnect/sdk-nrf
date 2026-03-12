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

list(APPEND src_crypto_cc3xx
  ./public_cc3xx_psa_asymmetric_signature.c
  ./public_cc3xx_psa_key_generation.c
)

add_library(cc3xx_psa_driver STATIC
  ${src_crypto_cc3xx}
)

# Link to the PSA interface and library PSA crypto configurations
target_link_libraries(cc3xx_psa_driver
  PRIVATE
    psa_crypto_library_config
    psa_interface
)

# Link to imported libraries for CC3XX APIs.
target_link_libraries(cc3xx_psa_driver
  PRIVATE
    nrf_cc3xx_psa_crypto_imported
    nrf_cc3xx_core_imported
)

# Link psa_core with this driver
target_link_libraries(psa_core
  PRIVATE
    cc3xx_psa_driver
)

# Link psa_core with the imported libraries
target_link_libraries(psa_core
  INTERFACE
    cc3xx_psa_driver
    nrf_cc3xx_psa_crypto_imported
    nrf_cc3xx_core_imported
    c
)

nrf_security_add_zephyr_options_library(cc3xx_psa_driver)
