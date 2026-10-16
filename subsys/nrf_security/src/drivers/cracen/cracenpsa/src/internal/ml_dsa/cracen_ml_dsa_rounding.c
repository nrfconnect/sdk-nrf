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

int32_t cracen_ml_dsa_use_hint(int32_t hint_bit, int32_t r, uint32_t gamma2)
{
	/** Note: Branching on gamma2, which is a public parameter with exactly two possible
	 *  values, so m is selected to avoid variable runtime division.
	 */
	int32_t m = (gamma2 == ML_DSA_GAMMA2(32)) ? ML_DSA_HIGH_BITS_MOD_32
						  : ML_DSA_HIGH_BITS_MOD_88;
	int32_t r0;
	int32_t r1;

	ml_dsa_decompose_reduced(r, gamma2, &r0, &r1);
	if (hint_bit == 1) {
		/* Mask for (r0 <= 0) */
		uint32_t r0_le_zero = (r0 - 1) >> 31;

		r1 += (r0_le_zero & (m - 1)) | (~r0_le_zero & 1u);
		/* subtract m when r1 >= m to avoid modulo operation */
		r1 -= ((m - 1 - r1) >> 31) & m;
	}

	return r1;
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

	cracen_ml_dsa_decompose(r, gamma2, &r0, &r1);
	return r1;
}
