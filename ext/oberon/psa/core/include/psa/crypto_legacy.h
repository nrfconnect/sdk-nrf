/**
 * \file psa/crypto_legacy.h
 *
 * \brief Add temporary suppport for deprecated symbols before they are
 *        removed from the library.
 *
 * PSA_WANT_KEY_TYPE_xxx_KEY_PAIR and MBEDTLS_PSA_ACCEL_KEY_TYPE_xxx_KEY_PAIR
 * symbols are deprecated.
 * New symols add a suffix to that base name in order to clearly state what is
 * the expected use for the key (use, import, export, generate, derive).
 * Here we define some backward compatibility support for uses stil using
 * the legacy symbols.
 */
/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef MBEDTLS_PSA_CRYPTO_LEGACY_H
#define MBEDTLS_PSA_CRYPTO_LEGACY_H

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR) //no-check-names
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_BASIC      1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_IMPORT   1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_EXPORT   1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_GENERATE 1
#endif
#if !defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define PSA_WANT_KEY_TYPE_ECC_KEY_PAIR_DERIVE   1
#endif
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR) //no-check-names
#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_BASIC      1
#endif
#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_IMPORT   1
#endif
#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_EXPORT   1
#endif
//#if !defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE)  /* !!OM */
//#define PSA_WANT_KEY_TYPE_RSA_KEY_PAIR_GENERATE 1
//#endif
#endif

#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR) //no-check-names
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_BASIC
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_IMPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_EXPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_GENERATE
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_ECC_KEY_PAIR_DERIVE
#endif
#endif

#if defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR) //no-check-names
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_BASIC)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_BASIC
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_IMPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_IMPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_EXPORT)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_EXPORT
#endif
#if !defined(MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_GENERATE)
#define MBEDTLS_PSA_ACCEL_KEY_TYPE_RSA_KEY_PAIR_GENERATE
#endif
#endif

#endif /* MBEDTLS_PSA_CRYPTO_LEGACY_H */
