#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Add nrf_cc3xx_platform includes for mbedcrypto
target_include_directories(psa_crypto_library_config
  INTERFACE
    $<TARGET_PROPERTY:nrf_cc3xx_platform,INTERFACE_INCLUDE_DIRECTORIES>
)

# Link the Oberon PSA core with nrf_cc3xx_platform
target_link_libraries(oberon_psa_core
  PRIVATE
    nrf_cc3xx_platform
)
