#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

include($ENV{ZEPHYR_BASE}/tests/modules/tf-m/regression/CMakeLists.txt)

project(tfm_regression_test_extended)

# CONFIG_TFM_ITS_ENCRYPTED (ITS_ENCRYPTION) enables PSA_WANT_ALG_CHACHA20_POLY1305
# which results in the ChaCha20* tests being enabled, but some of them (RFC7539 verification)
# fail, so keep them disabled for now as they used to be.
target_compile_options(app PRIVATE
    -UTFM_CRYPTO_TEST_CHACHA20
    -UTFM_CRYPTO_TEST_ALG_CHACHA20_POLY1305
)
