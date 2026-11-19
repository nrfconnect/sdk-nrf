#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_PSA_WANT_GENERATE_RANDOM AND NOT CONFIG_PSA_CRYPTO_DRIVER_ALG_PRNG_TEST AND NOT
   (CONFIG_PSA_NEED_CRACEN_CTR_DRBG_DRIVER OR
    CONFIG_PSA_NEED_OBERON_CTR_DRBG_DRIVER OR
    CONFIG_PSA_NEED_OBERON_HMAC_DRBG_DRIVER OR
    CONFIG_PSA_NEED_CC3XX_CTR_DRBG_DRIVER OR
    CONFIG_PSA_NEED_CC3XX_HMAC_DRBG_DRIVER))
  message(FATAL_ERROR "CONFIG_PSA_WANT_GENERATE_RANDOM is enabled, but no random driver is enabled")
endif()

if(CONFIG_PSA_CORE_OBERON)
  add_subdirectory(nrf_oberon)
elseif(CONFIG_PSA_CORE_LITE)
  add_subdirectory(lite)
else()
  message(FATAL_ERROR "CONFIG_MBEDTLS_PSA_CRYPTO_C enabled without PSA core")
endif()
