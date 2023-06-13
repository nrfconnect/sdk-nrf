/**
 * \file check_crypto_config.h
 *
 * \brief Consistency checks for PSA configuration options
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

/*
 * It is recommended to include this file from your crypto_config.h
 * in order to catch dependency issues early.
 */

#ifndef MBEDTLS_CHECK_CRYPTO_CONFIG_H
#define MBEDTLS_CHECK_CRYPTO_CONFIG_H

#if defined(PSA_WANT_ALG_CBC_NO_PADDING) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_CBC_NO_PADDING defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_CBC_PKCS7) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_CBC_PKCS7 defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_CCM) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_CCM defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_CCM_STAR_NO_TAG) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_CCM_STAR_NO_TAG defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_CMAC) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_CMAC defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_CTR) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_CTR defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_ECB_NO_PADDING) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_ECB_NO_PADDING defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_GCM) && \
    !( defined(PSA_WANT_KEY_TYPE_AES) )
#error "PSA_WANT_ALG_GCM defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_STREAM_CIPHER) && \
    !( defined(PSA_WANT_KEY_TYPE_CHACHA20) )
#error "PSA_WANT_ALG_STREAM_CIPHER defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_DETERMINISTIC_ECDSA) && \
    !( ( defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR) || \
         defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY) ) && \
       defined(PSA_WANT_ALG_HMAC) )
#error "PSA_WANT_ALG_DETERMINISTIC_ECDSA defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_ECDSA) && \
    !( defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR) || \
       defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY) )
#error "PSA_WANT_ALG_ECDSA defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_PURE_EDDSA) && \
    !( defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR) || \
       defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY) ) && \
    !( defined(PSA_WANT_ECC_TWISTED_EDWARDS_255) || \
       defined(PSA_WANT_ECC_TWISTED_EDWARDS_448) )
#error "PSA_WANT_ALG_ECDSA defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_ECDH) && \
    !( defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR) || \
       defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY) )
#error "PSA_WANT_ALG_ECDH defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_RSA_PKCS1V15_CRYPT) && \
    !( defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR) || \
       defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY) )
#error "PSA_WANT_ALG_RSA_PKCS1V15_CRYPT defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_RSA_PKCS1V15_SIGN) && \
    !( defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR) || \
       defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY) )
#error "PSA_WANT_ALG_RSA_PKCS1V15_SIGN defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_RSA_OAEP) && \
    !( defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR) || \
       defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY) )
#error "PSA_WANT_ALG_RSA_OAEP defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_RSA_PSS) && \
    !( defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR) || \
       defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY) )
#error "PSA_WANT_ALG_RSA_PSS defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_KEY_TYPE_ECC_KEY_PAIR) && \
    !defined(PSA_WANT_KEY_TYPE_ECC_PUBLIC_KEY)
#error "PSA_WANT_KEY_TYPE_ECC_KEY_PAIR defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_CMAC) && \
    !defined(PSA_WANT_ALG_ECB_NO_PADDING)
#error "PSA_WANT_ALG_CMAC defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_HKDF) && \
    !defined(PSA_WANT_ALG_HMAC)
#error "PSA_WANT_ALG_HKDF defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_HKDF_EXTRACT) && \
    !defined(PSA_WANT_ALG_HMAC)
#error "PSA_WANT_ALG_HKDF_EXTRACT defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_HKDF_EXPAND) && \
    !defined(PSA_WANT_ALG_HMAC)
#error "PSA_WANT_ALG_HKDF_EXPAND defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_TLS12_PRF) && \
    !defined(PSA_WANT_ALG_HMAC)
#error "PSA_WANT_ALG_TLS12_PRF defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_TLS12_PSK_TO_MS) && \
    !defined(PSA_WANT_ALG_HMAC)
#error "PSA_WANT_ALG_TLS12_PSK_TO_MS defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_PBKDF2_HMAC) && \
    !defined(PSA_WANT_ALG_HMAC)
#error "PSA_WANT_ALG_PBKDF2_HMAC defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128) && \
    !( defined(PSA_WANT_ALG_CMAC) && \
       defined(PSA_WANT_AES_KEY_SIZE_128))
#error "PSA_WANT_ALG_PBKDF2_AES_CMAC_PRF_128 defined, but not all prerequisites"
#endif

#if defined(PSA_WANT_KEY_TYPE_AES) && \
    !( defined(PSA_WANT_AES_KEY_SIZE_128) || \
       defined(PSA_WANT_AES_KEY_SIZE_192) || \
       defined(PSA_WANT_AES_KEY_SIZE_256) )
#error "PSA_WANT_KEY_TYPE_AES defined, but no AES key size"
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_KEY_PAIR) && \
    !( defined(PSA_WANT_RSA_KEY_SIZE_1024) || \
       defined(PSA_WANT_RSA_KEY_SIZE_1536) || \
       defined(PSA_WANT_RSA_KEY_SIZE_2048) || \
       defined(PSA_WANT_RSA_KEY_SIZE_3072) || \
       defined(PSA_WANT_RSA_KEY_SIZE_4096) || \
       defined(PSA_WANT_RSA_KEY_SIZE_6144) || \
       defined(PSA_WANT_RSA_KEY_SIZE_8192) )
#error "PSA_WANT_KEY_TYPE_RSA_KEY_PAIR defined, but no RSA key size"
#endif

#if defined(PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY) && \
    !( defined(PSA_WANT_RSA_KEY_SIZE_1024) || \
       defined(PSA_WANT_RSA_KEY_SIZE_1536) || \
       defined(PSA_WANT_RSA_KEY_SIZE_2048) || \
       defined(PSA_WANT_RSA_KEY_SIZE_3072) || \
       defined(PSA_WANT_RSA_KEY_SIZE_4096) || \
       defined(PSA_WANT_RSA_KEY_SIZE_6144) || \
       defined(PSA_WANT_RSA_KEY_SIZE_8192) )
#error "PSA_WANT_KEY_TYPE_RSA_PUBLIC_KEY defined, but no RSA key size"
#endif

#if defined(PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS) && \
    !defined(PSA_WANT_ALG_SHA_256)
#error "PSA_WANT_ALG_TLS12_ECJPAKE_TO_PMS defined, but not all prerequisites"
#endif

#endif /* MBEDTLS_CHECK_CRYPTO_CONFIG_H */
