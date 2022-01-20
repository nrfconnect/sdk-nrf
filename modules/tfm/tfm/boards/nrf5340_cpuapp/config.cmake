#-------------------------------------------------------------------------------
# Copyright (c) 2021, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(PLATFORM_PATH platform/ext/target/nordic_nrf/)
include(${PLATFORM_PATH}/common/nrf5340/config.cmake)

# Override the AEAD algorithm configuration
set(PS_CRYPTO_AEAD_ALG                  PSA_ALG_GCM CACHE STRING    "The AEAD algorithm to use for authenticated encryption in Protected Storage")
set(PLATFORM_DEFAULT_CRYPTO_KEYS        FALSE       CACHE BOOL      "Use default crypto keys implementation.")

# Disable crypto regression tests that are not supported
set(TFM_CRYPTO_TEST_ALG_CFB             OFF          CACHE BOOL      "Test CFB cryptography mode")
