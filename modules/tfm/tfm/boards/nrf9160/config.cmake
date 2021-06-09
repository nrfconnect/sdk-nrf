#-------------------------------------------------------------------------------
# Copyright (c) 2021, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

include(platform/ext/target/nordic_nrf/common/nrf9160/config.cmake)

# Override the AEAD algorithm configuration since nRF9160 supports only AES_CCM
set(PS_CRYPTO_AEAD_ALG                  PSA_ALG_CCM CACHE STRING    "The AEAD algorithm to use for authenticated encryption in Protected Storage")
set(PLATFORM_DUMMY_CRYPTO_KEYS          FALSE      CACHE BOOL      "Use dummy crypto keys. Should not be used in production.")
