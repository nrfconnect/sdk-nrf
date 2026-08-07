/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_rounding.h"

/** FIPS 204, Algorithm 36 (Decompose).
 *  Splits r (in [0, q)) into high bits r1 and the centered low bits r0
 *  in range (-gamma2, gamma2].
 *  Note: rounding range is gamma2 in terms of the spec.
 */
static void decompose(int32_t r, uint32_t gamma2, int32_t *r0, int32_t *r1)
{
	int32_t alpha = (int32_t)(2u * gamma2); /* See FIPS 204, Section 2.3 */
	int32_t low;
	int32_t high;

	low = r % alpha;
	/* low bits must be centered, i.e. be in the range (-gamma2/2; gamma2/2] */
	if (low > (int32_t)gamma2) {
		low -= alpha;
	}

	if (r - low == ML_DSA_PRIME_NUM - 1) {
		high = 0;
		low -= 1;
	} else {
		high = (r - low) / alpha;
	}

	*r0 = low;
	*r1 = high;
}

int32_t cracen_ml_dsa_use_hint(int32_t hint_bit, int32_t r, uint32_t gamma2)
{
	int32_t m = (int32_t)((ML_DSA_PRIME_NUM - 1) / (2u * gamma2));
	int32_t r0;
	int32_t r1;

	decompose(r, gamma2, &r0, &r1);
	if (hint_bit == 1) {
		if (r0 > 0) {
			r1 = (r1 + 1) % m;
		} else {
			r1 = (r1 - 1 + m) % m;
		}
	}

	return r1;
}
