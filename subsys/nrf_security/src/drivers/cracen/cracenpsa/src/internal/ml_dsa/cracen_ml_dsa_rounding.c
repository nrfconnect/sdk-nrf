/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_rounding.h"

/* Number of dropped low bits kept in t0 (2^(d-1)): the ExpandMask/BitPack bound
 * for t0 coefficients, which lie in [-(2^(d-1) - 1), 2^(d-1)].
 */
#define ML_DSA_T0_COEFF_BOUND	(1 << (ML_DSA_DROPPED_BITS_COUNT - 1))

/* The two possible values of gamma2 and the matching number of distinct high-bit
 * values m = (q - 1) / (2 * gamma2), see FIPS 204, Table 1.
 */
#define ML_DSA_HIGH_BITS_MOD_32	((ML_DSA_PRIME_NUM - 1) / (2 * ML_DSA_GAMMA2(32)))
#define ML_DSA_HIGH_BITS_MOD_88	((ML_DSA_PRIME_NUM - 1) / (2 * ML_DSA_GAMMA2(88)))

/** FIPS 204, Algorithm 36 (Decompose).
 *  Splits r (in [0, q)) into high bits r1 and the centered low bits r0
 *  in range (-gamma2, gamma2].
 *  Note: rounding range is gamma2 in terms of the spec.
 *
 *  r is secret-dependent, so this function avoids data-dependent division/modulo
 *  and branching on r: it uses the constant-time approximation inspired by the CRYSTALS-Dilithium
 *  reference implementation, which only branches on gamma2 (a public parameter with
 *  exactly two possible values).
 */
static void decompose(int32_t r, uint32_t gamma2, int32_t *r0, int32_t *r1)
{
	int32_t alpha = (int32_t)(gamma2 << 1); /* alpha = 2*gamma2, see FIPS 204, Section 2.3 */
	int32_t high;
	int32_t low;

	high = (r + 127) >> 7;
	/** Note: Branching on gamma2, which is a public parameter with exactly two possible values
	 *  (depends on algorithm type).
	 */
	if (gamma2 == ML_DSA_GAMMA2(32)) {
		high = (high * 1025 + (1 << 21)) >> 22;
		high &= 15;
	} else {
		high = (high * 11275 + (1 << 23)) >> 24;
		high ^= ((43 - high) >> 31) & high;
	}

	low = r - high * alpha;
	low -= (((ML_DSA_PRIME_NUM - 1) / 2 - low) >> 31) & ML_DSA_PRIME_NUM;

	*r0 = low;
	*r1 = high;
}

int32_t cracen_ml_dsa_use_hint(int32_t hint_bit, int32_t r, uint32_t gamma2)
{
	/** Note: Branching on gamma2, which is a public parameter with exactly two possible
	 *  values, so m is selected to avoid variable runtime division.
	 */
	int32_t m = (gamma2 == ML_DSA_GAMMA2(32)) ? ML_DSA_HIGH_BITS_MOD_32
						  : ML_DSA_HIGH_BITS_MOD_88;
	int32_t r0;
	int32_t r1;

	decompose(r, gamma2, &r0, &r1);
	if (hint_bit == 1) {
		/* Mask for (r0 <= 0) */
		uint32_t r0_le_zero = (r0 - 1) >> 31;

		r1 += (r0_le_zero & (m - 1)) | (~r0_le_zero & 1u);
		/* subtract m when r1 >= m to avoid modulo operation */
		r1 -= ((m - 1 - r1) >> 31) & m;
	}

	return r1;
}

/* Bring a coefficient in the range (-q, 2q) back into [0, q), as required by
 * decompose(). Signing forms these coefficients as sums and differences of
 * inverse-NTT outputs (each already in [0, q)).
 */
static int32_t center_to_zq(int32_t r)
{
	int32_t sum = ((r >> 31) & ML_DSA_PRIME_NUM) |
		       (((ML_DSA_PRIME_NUM - r - 1) >> 31) & -ML_DSA_PRIME_NUM);

	return r + sum;
}

void cracen_ml_dsa_power2round(const ml_dsa_poly_vector_t *in, ml_dsa_poly_vector_t *t1,
			       ml_dsa_poly_vector_t *t0)
{
	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		int32_t r = in->coeffs[i];
		/* r0 = r mod+- 2^d, centered into (-2^(d-1), 2^(d-1)]. */
		int32_t r0 = r & ((1 << ML_DSA_DROPPED_BITS_COUNT) - 1);

		r0 -= ((ML_DSA_T0_COEFF_BOUND - r0) >> 31) & (1 << ML_DSA_DROPPED_BITS_COUNT);

		t0->coeffs[i] = r0;
		t1->coeffs[i] = (r - r0) >> ML_DSA_DROPPED_BITS_COUNT;
	}
}

int32_t cracen_ml_dsa_high_bits(int32_t r, uint32_t gamma2)
{
	int32_t r0;
	int32_t r1;

	decompose(center_to_zq(r), gamma2, &r0, &r1);
	return r1;
}

int32_t cracen_ml_dsa_low_bits(int32_t r, uint32_t gamma2)
{
	int32_t r0;
	int32_t r1;

	decompose(center_to_zq(r), gamma2, &r0, &r1);
	return r0;
}

int32_t cracen_ml_dsa_make_hint(int32_t z, int32_t r, uint32_t gamma2)
{
	return cracen_ml_dsa_high_bits(r, gamma2) != cracen_ml_dsa_high_bits(r + z, gamma2);
}
