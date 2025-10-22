#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nrf_cc3xx_platform is not a driver in the old APIs. Once ctr_drbg/hmac_drbg
# gets real driver API support then the placement here makes more sense
if(TARGET nrf_cc3xx_platform)
  add_subdirectory(nrf_cc3xx_platform)
endif()

# Only build the PSA crypto drivers when Oberon 
if(CONFIG_MBEDTLS_PSA_CRYPTO_C)
  if(CONFIG_PSA_CRYPTO_DRIVER_CC3XX)
    add_subdirectory(nrf_cc3xx)
  endif()

  if(CONFIG_PSA_CRYPTO_DRIVER_CRACEN)
    add_subdirectory(cracen)
  endif()

  if(CONFIG_PSA_CRYPTO_DRIVER_OBERON)
    add_subdirectory(nrf_oberon)
  endif()

  add_subdirectory(zephyr)
endif()


# Includes need to be added regardless if PSA crypto drivers are built
if(NOT BUILD_INSIDE_TFM)
  add_subdirectory(legacy)
endif()
