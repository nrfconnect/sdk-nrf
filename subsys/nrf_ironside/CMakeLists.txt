#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_PSA_SSF_CRYPTO_CLIENT)

zephyr_library()
zephyr_library_sources(
  # ironside_psa_ns_api.c provides psa_call, which sends a message over IPC.
  # psa_call is invoked by serialized functions from tfm_crypto_api.c and tfm_its_api.c.
  ${CMAKE_CURRENT_LIST_DIR}/ironside_se_psa_ns_api.c
  # tfm_crypto_api.c provides and serializes the PSA Crypto API.
  ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/interface/src/tfm_crypto_api.c
  # tfm_its_api.c provides and serializes the PSA Internal Trusted Storage API.
  ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/interface/src/tfm_its_api.c
  )

zephyr_library_include_directories(
  .
  ${NRF_DIR}/include/tfm
  ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/interface/include
  # Oberon PSA headers
  ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include
  ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/library
  )

if(CONFIG_PSA_SSF_CRYPTO_CLIENT_OUT_BOUNCE_BUFFERS)
  zephyr_library_sources(
    ${CMAKE_CURRENT_LIST_DIR}/bounce_buffers.c
    )
endif()

zephyr_include_directories(
  .
  ${NRF_DIR}/include/tfm
  ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/interface/include
  # Oberon PSA headers
  ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include
  ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/library
)

zephyr_compile_definitions(MBEDTLS_PSA_CRYPTO_CONFIG_FILE="ironside_config.h")
zephyr_compile_definitions(MBEDTLS_CONFIG_FILE="ironside_config.h")

endif()
