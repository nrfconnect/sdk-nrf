/**
 * \file poly1305_ext.h
 *
 * \brief   This file contains Poly1305 definitions and functions.
 *
 *          Poly1305 is a one-time message authenticator that can be used to
 *          authenticate messages. Poly1305-AES was created by Daniel
 *          Bernstein https://cr.yp.to/mac/poly1305-20050329.pdf The generic
 *          Poly1305 algorithm (not tied to AES) was also standardized in RFC
 *          7539.
 *
 * \author Daniel King <damaki.gh@gmail.com>
 */

/*
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

/** Copied from MbedTLS, modified to contain required operations only.
 *
 *  Original file: poly1305.h
 *  Commit: 68788fe8eeafbdfde51997974cfe59c5581a4317
 *  Link: https://git.new/vnUJRck
 */

#ifndef TF_PSA_CRYPTO_MBEDTLS_PRIVATE_POLY1305_EXT_H
#define TF_PSA_CRYPTO_MBEDTLS_PRIVATE_POLY1305_EXT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct poly1305_ext_context {
    uint32_t r[4];      /** The value for 'r' (low 128 bits of the key). */
    uint32_t s[4];      /** The value for 's' (high 128 bits of the key). */
    uint32_t acc[5];    /** The accumulator number. */
}
poly1305_ext_context;

/**
 * \brief           This function sets the one-time authentication key.
 *
 * \warning         The key must be unique and unpredictable for each
 *                  invocation of Poly1305.
 *
 * \param ctx       The Poly1305 context to which the key should be bound.
 *                  This must be initialized.
 * \param key       The buffer containing the \c 32 Byte (\c 256 Bit) key.
 *
 * \return          \c 0 on success.
 * \return          A negative error code on failure.
 */
int poly1305_ext_starts(poly1305_ext_context *ctx,
                        const unsigned char key[32]);

/**
 * \brief                   Process blocks with Poly1305.
 *
 * \param ctx               The Poly1305 context.
 * \param nblocks           Number of blocks to process. Note that this
 *                          function only processes full blocks.
 * \param input             Buffer containing the input block(s).
 * \param needs_padding     Set to 0 if the padding bit has already been
 *                          applied to the input data before calling this
 *                          function.  Otherwise, set this parameter to 1.
 */
void poly1305_ext_process(poly1305_ext_context *ctx,
                          size_t nblocks,
                          const unsigned char *input,
                          uint32_t needs_padding);

/**
 * \brief                   Compute the Poly1305 MAC
 *
 * \param ctx               The Poly1305 context.
 * \param mac               The buffer to where the MAC is written. Must be
 *                          big enough to contain the 16-byte MAC.
 */
void poly1305_ext_compute_mac(const poly1305_ext_context *ctx,
                              unsigned char mac[16]);

#ifdef __cplusplus
}
#endif

#endif /* TF_PSA_CRYPTO_MBEDTLS_PRIVATE_POLY1305_EXT_H */
