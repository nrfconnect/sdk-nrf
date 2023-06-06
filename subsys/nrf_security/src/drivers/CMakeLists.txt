# nrf_cc3xx_platform is not a driver in the old APIs. Once ctr_drbg/hmac_drbg 
# gets real driver API support then the placement here makes more sense
if(TARGET nrf_cc3xx_platform)
  add_subdirectory(nrf_cc3xx_platform)
endif()

if(CONFIG_PSA_CRYPTO_DRIVER_BUILTIN)
  add_subdirectory(builtin)
endif()

if(CONFIG_PSA_CRYPTO_DRIVER_CC3XX AND NOT CONFIG_BUILD_WITH_TFM)
  add_subdirectory(nrf_cc3xx)
endif()

add_subdirectory(zephyr)

if(CONFIG_PSA_CRYPTO_DRIVER_OBERON OR CONFIG_OBERON_BACKEND)
  add_subdirectory(nrf_oberon)
endif()
