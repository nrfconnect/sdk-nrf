#-------------------------------------------------------------------------------
# Copyright (c) 2021, Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

include(platform/ext/target/nordic_nrf/common/nrf5340/config.cmake)

# Override the AEAD algorithm configuration
set(PS_CRYPTO_AEAD_ALG                  PSA_ALG_GCM CACHE STRING    "The AEAD algorithm to use for authenticated encryption in Protected Storage")
