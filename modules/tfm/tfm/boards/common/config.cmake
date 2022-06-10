#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Override the AEAD algorithm configuration
set(PLATFORM_DEFAULT_CRYPTO_KEYS        FALSE       CACHE BOOL      "Use default crypto keys implementation.")

# Disable crypto regression tests that are not supported
set(TFM_CRYPTO_TEST_ALG_CFB             OFF         CACHE BOOL      "Test CFB cryptography mode")
set(TFM_CRYPTO_TEST_ALG_OFB             OFF         CACHE BOOL      "Test OFB cryptography mode")

# Always enable the nonsecure storage partition.
# It will still be excluded if the partition manager excludes it.
set(NRF_NS_STORAGE                      ON          CACHE BOOL      "Enable non-secure storage partition")
