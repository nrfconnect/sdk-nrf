/*
 *  NIST SP800-38D compliant GCM implementation
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

/*
 * http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
 *
 * See also:
 * [MGV] http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/gcm/gcm-revised-spec.pdf
 *
 * We use the algorithm described as Shoup's method with 4-bit tables in
 * [MGV] 4.1, pp. 12-13, to enhance speed without using too much memory.
 */

/* Copied from mbed TLS, modified to contain GF(2^128) operation only */

#include "common.h"
#include "gcm_ext.h"

#include <string.h>

static inline void gcm_gen_table_rightshift(uint64_t dst[2], const uint64_t src[2])
{
	uint8_t *u8Dst = (uint8_t *) dst;
	uint8_t *u8Src = (uint8_t *) src;

	MBEDTLS_PUT_UINT64_BE(MBEDTLS_GET_UINT64_BE(&src[1], 0) >> 1, &dst[1], 0);
	u8Dst[8] |= (u8Src[7] & 0x01) << 7;
	MBEDTLS_PUT_UINT64_BE(MBEDTLS_GET_UINT64_BE(&src[0], 0) >> 1, &dst[0], 0);
	u8Dst[0] ^= (u8Src[15] & 0x01) ? 0xE1 : 0;
}

/*
 * Precompute small multiples of H, that is set
 *      HH[i] || HL[i] = H times i,
 * where i is seen as a field element as in [MGV], ie high-order bits
 * correspond to low powers of P. The result is stored in the same way, that
 * is the high-order bit of HH corresponds to P^0 and the low-order bit of HL
 * corresponds to P^127.
 */
int gcm_ext_gen_table(const uint8_t *h, uint64_t H[16][2])
{
	int i, j;
	const uint64_t *u64h = (const uint64_t *)h;

	/* GCM_EXT_HTABLE_SIZE/2 = 1000 corresponds to 1 in GF(2^128) */
	H[GCM_EXT_HTABLE_SIZE/2][0] = u64h[0];
	H[GCM_EXT_HTABLE_SIZE/2][1] = u64h[1];

	/* 0 corresponds to 0 in GF(2^128) */
	H[0][0] = 0;
	H[0][1] = 0;

	for (i = GCM_EXT_HTABLE_SIZE/4; i > 0; i >>= 1) {
		gcm_gen_table_rightshift(H[i], H[i*2]);
	}

	/* Note: only small table implementation is used */
	/* pack elements of H as 64-bits ints, big-endian */
	for (i = GCM_EXT_HTABLE_SIZE/2; i > 0; i >>= 1) {
		MBEDTLS_PUT_UINT64_BE(H[i][0], &H[i][0], 0);
		MBEDTLS_PUT_UINT64_BE(H[i][1], &H[i][1], 0);
	}

	for (i = 2; i < GCM_EXT_HTABLE_SIZE; i <<= 1) {
		for (j = 1; j < i; j++) {
			mbedtls_xor_no_simd((unsigned char *) H[i+j],
					    (unsigned char *) H[i],
					    (unsigned char *) H[j],
					    16);
		}
	}

	return 0;
}

/*
 * Shoup's method for multiplication use this table with
 *      last4[x] = x times P^128
 * where x and last4[x] are seen as elements of GF(2^128) as in [MGV]
 */
static const uint16_t last4[16] =
{
	0x0000, 0x1c20, 0x3840, 0x2460,
	0x7080, 0x6ca0, 0x48c0, 0x54e0,
	0xe100, 0xfd20, 0xd940, 0xc560,
	0x9180, 0x8da0, 0xa9c0, 0xb5e0
};

static void gcm_mult_smalltable(uint8_t *output, const uint8_t *x, const uint64_t H[16][2])
{
	int i = 0;
	unsigned char lo, hi, rem;
	uint64_t u64z[2];
	const uint64_t *pu64z = NULL;
	uint8_t *u8z = (uint8_t *) u64z;

	lo = x[15] & 0xf;
	hi = (x[15] >> 4) & 0xf;

	pu64z = H[lo];

	rem = (unsigned char) pu64z[1] & 0xf;
	u64z[1] = (pu64z[0] << 60) | (pu64z[1] >> 4);
	u64z[0] = (pu64z[0] >> 4);
	u64z[0] ^= (uint64_t) last4[rem] << 48;
	mbedtls_xor_no_simd(u8z, u8z, (uint8_t *) H[hi], 16);

	for (i = 14; i >= 0; i--) {
		lo = x[i] & 0xf;
		hi = (x[i] >> 4) & 0xf;

		rem = (unsigned char) u64z[1] & 0xf;
		u64z[1] = (u64z[0] << 60) | (u64z[1] >> 4);
		u64z[0] = (u64z[0] >> 4);
		u64z[0] ^= (uint64_t) last4[rem] << 48;
		mbedtls_xor_no_simd(u8z, u8z, (uint8_t *) H[lo], 16);

		rem = (unsigned char) u64z[1] & 0xf;
		u64z[1] = (u64z[0] << 60) | (u64z[1] >> 4);
		u64z[0] = (u64z[0] >> 4);
		u64z[0] ^= (uint64_t) last4[rem] << 48;
		mbedtls_xor_no_simd(u8z, u8z, (uint8_t *) H[hi], 16);
	}

	MBEDTLS_PUT_UINT64_BE(u64z[0], output, 0);
	MBEDTLS_PUT_UINT64_BE(u64z[1], output, 8);
}

/*
 * Sets output to x times H using the precomputed tables.
 * x and output are seen as elements of GF(2^128) as in [MGV].
 */
void gcm_ext_mult(const uint64_t H[16][2], const unsigned char x[16],
		  unsigned char output[16])
{
	gcm_mult_smalltable(output, x, H);
	return;
}
