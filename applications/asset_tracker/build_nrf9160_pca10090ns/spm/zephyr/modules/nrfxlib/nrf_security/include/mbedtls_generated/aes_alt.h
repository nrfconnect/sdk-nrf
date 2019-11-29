/*
 * Copyright (c) 2001-2019, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause OR Arm’s non-OSI source license
 */

#ifndef MBEDTLS_AES_ALT_H
#define MBEDTLS_AES_ALT_H


#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif


#include <stddef.h>
#include <stdint.h>

#if defined(MBEDTLS_AES_ALT)


/* padlock.c and aesni.c rely on these values! */
#define MBEDTLS_AES_ENCRYPT     1 /**< AES encryption. */
#define MBEDTLS_AES_DECRYPT     0 /**< AES decryption. */

/* Error codes in range 0x0020-0x0022 */
#define MBEDTLS_ERR_AES_INVALID_KEY_LENGTH                -0x0020  /**< Invalid key length. */
#define MBEDTLS_ERR_AES_INVALID_INPUT_LENGTH              -0x0022  /**< Invalid data input length. */

/* Error codes in range 0x0023-0x0025 */
#define MBEDTLS_ERR_AES_FEATURE_UNAVAILABLE               -0x0023  /**< Feature not available. For example, an unsupported AES key size. */
#define MBEDTLS_ERR_AES_HW_ACCEL_FAILED                   -0x0025  /**< AES hardware accelerator failed. */


/* The Size of the AES context.*/
#define MBEDTLS_AES_CONTEXT_SIZE_IN_WORDS           (24)


#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          AES context structure
 *
 */
typedef struct
{
    uint32_t buf[MBEDTLS_AES_CONTEXT_SIZE_IN_WORDS];
} mbedtls_aes_context;


#if defined(MBEDTLS_CIPHER_MODE_XTS)
/**
 * \brief The AES XTS context-type definition.
 */
typedef struct mbedtls_aes_xts_context
{
    mbedtls_aes_context crypt; /*!< The AES context to use for AES block
                                        encryption or decryption. */
    mbedtls_aes_context tweak; /*!< The AES context used for tweak
                                        computation. */
} mbedtls_aes_xts_context;
#endif /* MBEDTLS_CIPHER_MODE_XTS */

#ifdef __cplusplus
}
#endif

#endif  /* MBEDTLS_AES_ALT */

#endif /* MBEDTLS_AES_ALT_H */
