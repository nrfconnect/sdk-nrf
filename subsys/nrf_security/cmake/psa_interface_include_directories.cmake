#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(psa_interface
  INTERFACE
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/include
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/core
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/dispatch
    ${ZEPHYR_OBERON_PSA_CRYPTO_MODULE_DIR}/platform
    ${NRF_SECURITY_DIR}/include
)
