#
# Copyright (c) 2021-2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
get_target_property(NRF_CC3XX_MBEDCRYPTO_INCLUDE_PATH nrf_cc3xx_core_imported INTERFACE_INCLUDE_DIRECTORIES)

# Link to library containing PSA Crypto Driver
target_link_libraries(${mbedcrypto_target}
  INTERFACE
    nrf_cc3xx_psa_crypto_imported
    nrf_cc3xx_core_imported
    c
)

# Add nrf_cc3xx PSA Crypto Driver includes to mbedcrypto_common
target_include_directories(mbedcrypto_common
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${NRF_CC3XX_MBEDCRYPTO_INCLUDE_PATH}
)
