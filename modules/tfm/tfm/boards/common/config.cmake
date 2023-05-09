#
# Copyright (c) 2021 - 2023, Nordic Semiconductor ASA.
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
set(PLATFORM_DEFAULT_ATTEST_HAL         OFF         CACHE BOOL      "Use default attest hal implementation.")

set(NRF_ALLOW_NON_SECURE_RESET          OFF         CACHE BOOL      "Allow system reset calls from Non-Secure")

set(TFM_DUMMY_PROVISIONING              OFF         CACHE BOOL      "Provision with dummy values. NOT to be used in production")
set(PLATFORM_DEFAULT_PROVISIONING       OFF         CACHE BOOL      "Use default provisioning implementation")
set(NRF_PROVISIONING                    OFF         CACHE BOOL      "Use Nordic provisioning implementation")
set(CONFIG_NFCT_PINS_AS_GPIOS           OFF         CACHE BOOL      "Use NFCT pins as GPIOs.")
set(CONFIG_NRF_TRACE_PORT               OFF         CACHE BOOL      "Enable trace port.")
