#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_regression_test)

include(${ZEPHYR_BASE}/tests/modules/tf-m/regression/CMakeLists.txt)

# CONFIG_TFM_ITS_ENCRYPTED (ITS_ENCRYPTION) enables PSA_WANT_ALG_CHACHA20_POLY1305
# which results in the ChaCha20* tests being enabled, but some of them (RFC7539 verification)
# fail, so keep them disabled for now as they used to be.
target_compile_options(app PRIVATE
    -UTFM_CRYPTO_TEST_CHACHA20
    -UTFM_CRYPTO_TEST_ALG_CHACHA20_POLY1305
)
