/**
 * \file poly1305.c
 *
 * \brief Poly1305 authentication algorithm.
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

/** Copied from MbedTLS, modified to contain required operations only.
 *
 *  Original file: poly1305.c
 *  Commit: 9c7466827e2404f2d421a256282a3e19cec469c0
 *  Link: https://github.com/Mbed-TLS/TF-PSA-Crypto/blob/development/drivers/builtin/src/poly1305.c
 */

#include "common.h"
#include "poly1305_ext.h"

#define POLY1305_BLOCK_SIZE_BYTES (16U)

static inline uint64_t mul64(uint32_t a, uint32_t b)
{
    return (uint64_t) a * b;
}

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
                          uint32_t needs_padding)
{
    uint64_t d0, d1, d2, d3;
    uint32_t acc0, acc1, acc2, acc3, acc4;
    uint32_t r0, r1, r2, r3;
    uint32_t rs1, rs2, rs3;
    size_t offset  = 0U;
    size_t i;

    r0 = ctx->r[0];
    r1 = ctx->r[1];
    r2 = ctx->r[2];
    r3 = ctx->r[3];

    rs1 = r1 + (r1 >> 2U);
    rs2 = r2 + (r2 >> 2U);
    rs3 = r3 + (r3 >> 2U);

    acc0 = ctx->acc[0];
    acc1 = ctx->acc[1];
    acc2 = ctx->acc[2];
    acc3 = ctx->acc[3];
    acc4 = ctx->acc[4];

    /* Process full blocks */
    for (i = 0U; i < nblocks; i++) {
        /* The input block is treated as a 128-bit little-endian integer */
        d0   = MBEDTLS_GET_UINT32_LE(input, offset + 0);
        d1   = MBEDTLS_GET_UINT32_LE(input, offset + 4);
        d2   = MBEDTLS_GET_UINT32_LE(input, offset + 8);
        d3   = MBEDTLS_GET_UINT32_LE(input, offset + 12);

        /* Compute: acc += (padded) block as a 130-bit integer */
        d0  += (uint64_t) acc0;
        d1  += (uint64_t) acc1 + (d0 >> 32U);
        d2  += (uint64_t) acc2 + (d1 >> 32U);
        d3  += (uint64_t) acc3 + (d2 >> 32U);
        acc0 = (uint32_t) d0;
        acc1 = (uint32_t) d1;
        acc2 = (uint32_t) d2;
        acc3 = (uint32_t) d3;
        acc4 += (uint32_t) (d3 >> 32U) + needs_padding;

        /* Compute: acc *= r */
        d0 = mul64(acc0, r0) +
             mul64(acc1, rs3) +
             mul64(acc2, rs2) +
             mul64(acc3, rs1);
        d1 = mul64(acc0, r1) +
             mul64(acc1, r0) +
             mul64(acc2, rs3) +
             mul64(acc3, rs2) +
             mul64(acc4, rs1);
        d2 = mul64(acc0, r2) +
             mul64(acc1, r1) +
             mul64(acc2, r0) +
             mul64(acc3, rs3) +
             mul64(acc4, rs2);
        d3 = mul64(acc0, r3) +
             mul64(acc1, r2) +
             mul64(acc2, r1) +
             mul64(acc3, r0) +
             mul64(acc4, rs3);
        acc4 *= r0;

        /* Compute: acc %= (2^130 - 5) (partial remainder) */
        d1 += (d0 >> 32);
        d2 += (d1 >> 32);
        d3 += (d2 >> 32);
        acc0 = (uint32_t) d0;
        acc1 = (uint32_t) d1;
        acc2 = (uint32_t) d2;
        acc3 = (uint32_t) d3;
        acc4 = (uint32_t) (d3 >> 32) + acc4;

        d0 = (uint64_t) acc0 + (acc4 >> 2) + (acc4 & 0xFFFFFFFCU);
        acc4 &= 3U;
        acc0 = (uint32_t) d0;
        d0 = (uint64_t) acc1 + (d0 >> 32U);
        acc1 = (uint32_t) d0;
        d0 = (uint64_t) acc2 + (d0 >> 32U);
        acc2 = (uint32_t) d0;
        d0 = (uint64_t) acc3 + (d0 >> 32U);
        acc3 = (uint32_t) d0;
        d0 = (uint64_t) acc4 + (d0 >> 32U);
        acc4 = (uint32_t) d0;

        offset    += POLY1305_BLOCK_SIZE_BYTES;
    }

    ctx->acc[0] = acc0;
    ctx->acc[1] = acc1;
    ctx->acc[2] = acc2;
    ctx->acc[3] = acc3;
    ctx->acc[4] = acc4;
}

/**
 * \brief                   Compute the Poly1305 MAC
 *
 * \param ctx               The Poly1305 context.
 * \param mac               The buffer to where the MAC is written. Must be
 *                          big enough to contain the 16-byte MAC.
 */
void poly1305_ext_compute_mac(const poly1305_ext_context *ctx,
                              unsigned char mac[16])
{
    uint64_t d;
    uint32_t g0, g1, g2, g3, g4;
    uint32_t acc0, acc1, acc2, acc3, acc4;
    uint32_t mask;
    uint32_t mask_inv;

    acc0 = ctx->acc[0];
    acc1 = ctx->acc[1];
    acc2 = ctx->acc[2];
    acc3 = ctx->acc[3];
    acc4 = ctx->acc[4];

    /* Before adding 's' we ensure that the accumulator is mod 2^130 - 5.
     * We do this by calculating acc - (2^130 - 5), then checking if
     * the 131st bit is set. If it is, then reduce: acc -= (2^130 - 5)
     */

    /* Calculate acc + -(2^130 - 5) */
    d  = ((uint64_t) acc0 + 5U);
    g0 = (uint32_t) d;
    d  = ((uint64_t) acc1 + (d >> 32));
    g1 = (uint32_t) d;
    d  = ((uint64_t) acc2 + (d >> 32));
    g2 = (uint32_t) d;
    d  = ((uint64_t) acc3 + (d >> 32));
    g3 = (uint32_t) d;
    g4 = acc4 + (uint32_t) (d >> 32U);

    /* mask == 0xFFFFFFFF if 131st bit is set, otherwise mask == 0 */
    mask = (uint32_t) 0U - (g4 >> 2U);
    mask_inv = ~mask;

    /* If 131st bit is set then acc=g, otherwise, acc is unmodified */
    acc0 = (acc0 & mask_inv) | (g0 & mask);
    acc1 = (acc1 & mask_inv) | (g1 & mask);
    acc2 = (acc2 & mask_inv) | (g2 & mask);
    acc3 = (acc3 & mask_inv) | (g3 & mask);

    /* Add 's' */
    d = (uint64_t) acc0 + ctx->s[0];
    acc0 = (uint32_t) d;
    d = (uint64_t) acc1 + ctx->s[1] + (d >> 32U);
    acc1 = (uint32_t) d;
    d = (uint64_t) acc2 + ctx->s[2] + (d >> 32U);
    acc2 = (uint32_t) d;
    acc3 += ctx->s[3] + (uint32_t) (d >> 32U);

    /* Compute MAC (128 least significant bits of the accumulator) */
    MBEDTLS_PUT_UINT32_LE(acc0, mac,  0);
    MBEDTLS_PUT_UINT32_LE(acc1, mac,  4);
    MBEDTLS_PUT_UINT32_LE(acc2, mac,  8);
    MBEDTLS_PUT_UINT32_LE(acc3, mac, 12);
}

int poly1305_ext_starts(poly1305_ext_context *ctx,
                        const unsigned char key[32])
{
    /* r &= 0x0ffffffc0ffffffc0ffffffc0fffffff */
    ctx->r[0] = MBEDTLS_GET_UINT32_LE(key, 0)  & 0x0FFFFFFFU;
    ctx->r[1] = MBEDTLS_GET_UINT32_LE(key, 4)  & 0x0FFFFFFCU;
    ctx->r[2] = MBEDTLS_GET_UINT32_LE(key, 8)  & 0x0FFFFFFCU;
    ctx->r[3] = MBEDTLS_GET_UINT32_LE(key, 12) & 0x0FFFFFFCU;

    ctx->s[0] = MBEDTLS_GET_UINT32_LE(key, 16);
    ctx->s[1] = MBEDTLS_GET_UINT32_LE(key, 20);
    ctx->s[2] = MBEDTLS_GET_UINT32_LE(key, 24);
    ctx->s[3] = MBEDTLS_GET_UINT32_LE(key, 28);

    /* Initial accumulator state */
    ctx->acc[0] = 0U;
    ctx->acc[1] = 0U;
    ctx->acc[2] = 0U;
    ctx->acc[3] = 0U;
    ctx->acc[4] = 0U;

    return 0;
}
