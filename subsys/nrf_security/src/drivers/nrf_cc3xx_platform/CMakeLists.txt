#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Get include directories for nrf_cc3xx_platform
get_target_property(NRF_CC3XX_PLATFORM_INCLUDE_DIRECTORIES
  nrf_cc3xx_platform INTERFACE_INCLUDE_DIRECTORIES
)

# Add nrf_cc3xx_platform includes for mbedcrypto
target_include_directories(psa_crypto_library_config
  INTERFACE
    ${NRF_CC3XX_PLATFORM_INCLUDE_DIRECTORIES}
)

# Link the Oberon PSA core with nrf_cc3xx_platform
target_link_libraries(oberon_psa_core
  PRIVATE
    nrf_cc3xx_platform
)
