#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

append_with_prefix(src_crypto_core_oberon ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/platform/
  platform.c
  platform_util.c
)

append_with_prefix(src_crypto_core_oberon ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/utilities
  constant_time.c
)

append_with_prefix(src_crypto_core_oberon ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/core/
  psa_crypto.c
  psa_crypto_client.c
  psa_crypto_slot_management.c
  psa_crypto_storage.c
)

append_with_prefix(src_crypto_core_oberon ${NRF_SECURITY_DIR}/src/
  psa_crypto_driver_wrappers.c
)

if(CONFIG_MBEDTLS_PSA_CRYPTO_BUILTIN_KEYS)
  append_with_prefix(src_crypto_core_oberon ${NRF_SECURITY_DIR}/src/
    mbedtls_psa_platform.c
  )
endif()

add_library(psa_core STATIC
    ${src_crypto_core_oberon}
)

target_compile_definitions(psa_core
  PRIVATE
    BUILDING_PSA_CRYPTO # for Secure Storage
)

target_link_libraries(psa_core
  PRIVATE
    psa_crypto_library_config
    psa_interface
    nrf_security_utils
)

if(BUILD_INSIDE_TFM AND CRYPTO_TFM_BUILTIN_KEYS_DRIVER)
  # psa_crypto.c includes "tfm_builtin_key_loader.h" which
  # lives in TF-M's secure_fw/partitions/crypto/psa_driver_api/,
  # which is exported PUBLIC by crypto_service_tfpsacrypto — but psa_core
  # does not link against it. Add the include path directly.
  target_include_directories(psa_core
    PRIVATE
      ${CMAKE_SOURCE_DIR}/secure_fw/partitions/crypto/psa_driver_api
  )
endif()

nrf_security_add_zephyr_options_library(psa_core)

target_link_libraries(${tfpsacrypto_target}
  PRIVATE
    psa_core
)
