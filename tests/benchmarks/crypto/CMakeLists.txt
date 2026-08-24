#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(crypto_benchmarks)

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE
        src/main.c
        src/runner.c
        src/suites.c)

if(CONFIG_CRYPTO_BENCHMARKS_PSA_CIPHER)
        add_subdirectory(src/suites/psa_cipher)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_AEAD)
        add_subdirectory(src/suites/psa_aead)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_HASH)
        add_subdirectory(src/suites/psa_hash)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_XOF)
        add_subdirectory(src/suites/psa_xof)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_MAC)
        add_subdirectory(src/suites/psa_mac)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_DERIVATION)
        add_subdirectory(src/suites/psa_key_derivation)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_AGREEMENT)
        add_subdirectory(src/suites/psa_key_agreement)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_SIGNATURE)
        add_subdirectory(src/suites/psa_signature)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_ASYMMETRIC_ENCRYPTION)
        add_subdirectory(src/suites/psa_asymmetric_encryption)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_PAKE)
        add_subdirectory(src/suites/psa_pake)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_KEY_WRAP)
        add_subdirectory(src/suites/psa_key_wrap)
endif()
if(CONFIG_CRYPTO_BENCHMARKS_PSA_RNG)
        add_subdirectory(src/suites/psa_rng)
endif()
