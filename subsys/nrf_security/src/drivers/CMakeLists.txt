#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# If the PSA core is not built, then drivers are not needed
if(NOT (CONFIG_MBEDTLS_PSA_CRYPTO_C))
  return()
endif()

# nrf_cc3xx_platform is not a driver in the old APIs. Once ctr_drbg/hmac_drbg
# gets real driver API support then the placement here makes more sense
if(TARGET nrf_cc3xx_platform)
  add_subdirectory(nrf_cc3xx_platform)
endif()

if(CONFIG_PSA_CRYPTO_DRIVER_CC3XX)
  add_subdirectory(nrf_cc3xx)
endif()

if(CONFIG_PSA_CRYPTO_DRIVER_CRACEN)
  add_subdirectory(cracen)
endif()

add_subdirectory(zephyr)

if(CONFIG_PSA_CRYPTO_DRIVER_OBERON)
  add_subdirectory(nrf_oberon)
endif()
