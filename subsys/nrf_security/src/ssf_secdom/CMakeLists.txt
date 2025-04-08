#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_SSF_V_2)
target_sources(${mbedcrypto_target}
  PRIVATE
  # sdfw_psa_ns_api.c provides psa_call. psa_call is invoked by
  # serialized functions from tfm_crypto_api.c and sends a message
  # over IPC.
  ${CMAKE_CURRENT_LIST_DIR}/sdfw_psa_ns_api.c
  # sdfw_psa_ns_ipc.c provides an IPC service to sdfw_psa_ns_api.c
  ${CMAKE_CURRENT_LIST_DIR}/sdfw_psa_ns_ipc.c
  # tfm_crypto_api.c provides and serializes the PSA Crypto API.
  ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/interface/src/tfm_crypto_api.c
  )

target_include_directories(${mbedcrypto_target}
  PRIVATE
  ${NRF_DIR}/include/tfm
  .
  )

if(CONFIG_PSA_SSF_CRYPTO_CLIENT_OUT_BOUNCE_BUFFERS)
  target_sources(${mbedcrypto_target}
	PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/bounce_buffers.c
	)
endif()

else()
target_sources(${mbedcrypto_target}
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ssf_crypto.c
    ${CMAKE_CURRENT_LIST_DIR}/ssf_psa_core_compatibility.c
)
endif()
