/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_poly.h"

#include <zephyr/sys/byteorder.h>

/* q^(-1) mod 2^32, used by the Montgomery reduction */
#define ML_DSA_QINV	58728449

/* zeta[k] * R mod q (Montgomery form of the bit-reversed powers of the primitive
 * root, R = 2^32), i.e. zetas[k] = 1753^brv(k) * 2^32 mod q. Pre-scaling by R
 * lets cracen_ml_dsa_ntt() and cracen_ml_dsa_ntt_inversed() multiply by a zeta
 * with a single montgomery_reduce() call.
 */
static const int32_t zetas[] = {
	0x000000, 0x0064F7, 0x581103, 0x77F504, 0x039E44, 0x740119, 0x728129, 0x071E24,
	0x1BDE2B, 0x23E92B, 0x7A64AE, 0x5FF480, 0x2F9A75, 0x53DB0A, 0x2F7A49, 0x28E527,
	0x299658, 0x0FA070, 0x6F65A5, 0x36B788, 0x777D91, 0x6ECAA1, 0x27F968, 0x5FB37C,
	0x5F8DD7, 0x44FAE8, 0x6A84F8, 0x4DDC99, 0x1AD035, 0x7F9423, 0x3D3201, 0x0445C5,
	0x294A67, 0x017620, 0x2EF4CD, 0x35DEC5, 0x668504, 0x49102D, 0x5927D5, 0x3BBEAF,
	0x44F586, 0x516E7D, 0x368A96, 0x541E42, 0x360400, 0x7B4A4E, 0x23D69C, 0x77A55E,
	0x65F23E, 0x66CAD7, 0x357E1E, 0x458F5A, 0x35843F, 0x5F3618, 0x67745D, 0x38738C,
	0x0C63A8, 0x081B9A, 0x0E8F76, 0x3B3853, 0x3B8534, 0x58DC31, 0x1F9D54, 0x552F2E,
	0x43E6E6, 0x688C82, 0x47C1D0, 0x51781A, 0x69B65E, 0x3509EE, 0x2135C7, 0x67AFBC,
	0x6CAF76, 0x1D9772, 0x419073, 0x709CF7, 0x4F3281, 0x4FB2AF, 0x4870E1, 0x01EFCA,
	0x3410F2, 0x70DE86, 0x20C638, 0x296E9F, 0x5297A4, 0x47844C, 0x799A6E, 0x5A140A,
	0x75A283, 0x6D2114, 0x7F863C, 0x6BE9F8, 0x7A0BDE, 0x1495D4, 0x1C4563, 0x6A0C63,
	0x4CDBEA, 0x040AF0, 0x07C417, 0x2F4588, 0x00AD00, 0x6F16BF, 0x0DCD44, 0x3C675A,
	0x470BCB, 0x7FBE7F, 0x193948, 0x4E49C1, 0x24756C, 0x7CA7E0, 0x0B98A1, 0x6BC809,
	0x02E46C, 0x49A809, 0x3036C2, 0x639FF7, 0x5B1C94, 0x7D2AE1, 0x141305, 0x147792,
	0x139E25, 0x67B0E1, 0x737945, 0x69E803, 0x51CEA3, 0x44A79D, 0x488058, 0x3A97D9,
	0x1FEA93, 0x33FF5A, 0x2358D4, 0x3A41F8, 0x4CDF73, 0x223DFB, 0x5A8BA0, 0x498423,
	0x0412F5, 0x252587, 0x6D04F1, 0x359B5D, 0x4A28A1, 0x4682FD, 0x6D9B57, 0x4F25DF,
	0x0DBE5E, 0x1C5E1A, 0x0DE0E6, 0x0C7F5A, 0x078F83, 0x67428B, 0x7F3705, 0x77E6FD,
	0x75E022, 0x503AF7, 0x1F0084, 0x30EF86, 0x49997E, 0x77DCD7, 0x742593, 0x4901C3,
	0x053919, 0x04610C, 0x5AAD42, 0x3EB01B, 0x3472E7, 0x4CE03C, 0x1A7CC7, 0x031924,
	0x2B5EE5, 0x291199, 0x585A3B, 0x134D71, 0x3DE11C, 0x130984, 0x25F051, 0x185A46,
	0x466519, 0x1314BE, 0x283891, 0x49BB91, 0x52308A, 0x1C853F, 0x1D0B4B, 0x6FD6A7,
	0x6B88BF, 0x12E11B, 0x4D3E3F, 0x6A0D30, 0x78FDE5, 0x1406C7, 0x327283, 0x61ED6F,
	0x6C5954, 0x1D4099, 0x590579, 0x6AE5AE, 0x16E405, 0x0BDBE7, 0x221DE8, 0x33F8CF,
	0x779935, 0x54AA0D, 0x665FF9, 0x63B158, 0x58711C, 0x470C13, 0x0910D8, 0x463E20,
	0x612659, 0x251D8B, 0x2573B7, 0x7D5C90, 0x1DDD98, 0x336898, 0x02D4BB, 0x6D73A8,
	0x4F4CBF, 0x027C1C, 0x18AA08, 0x2DFD71, 0x0C5CA5, 0x19379A, 0x478168, 0x646C3E,
	0x51813D, 0x35C539, 0x3B0115, 0x041DC0, 0x21C4F7, 0x70FBF5, 0x1A35E7, 0x07340E,
	0x795D46, 0x1A4CD0, 0x645CAF, 0x1D2668, 0x666E99, 0x6F0634, 0x7BE5DB, 0x455FDC,
	0x530765, 0x5DC1B0, 0x7973DE, 0x5CFD0A, 0x02CC93, 0x70F806, 0x189C2A, 0x49C5AA,
	0x776A51, 0x3BCF2C, 0x7F234F, 0x6B16E0, 0x3C15CA, 0x155E68, 0x72F6B7, 0x1E29CE
};

/* Reduce a value with |a| <= 2^31 - 2^22 - 1 to r == a (mod q) with
 * |r| <= 6283008 < q. Costs one shift and one multiplication, unlike the
 * 64-bit division library call the % operator needs on Cortex-M.
 */
static int32_t reduce32(int32_t a)
{
	int32_t t = (a + (1 << 22)) >> 23;

	return a - t * ML_DSA_PRIME_NUM;
}

/* Compute a * R^(-1) mod q for R = 2^32, given |a| < 2^31 * q.
 * Result lies in (-q, q).
 */
static int32_t montgomery_reduce(int64_t a)
{
	int32_t t = (int32_t)((uint32_t)(int32_t)a * ML_DSA_QINV);

	return (int32_t)((a - (int64_t)t * ML_DSA_PRIME_NUM) >> 32);
}

void cracen_ml_dsa_ntt(ml_dsa_poly_vector_t *vec)
{
	int32_t *w = vec->coeffs;
	uint8_t k = 1;

	for (uint32_t len = 128; len >= 1; len >>= 1) {
		for (uint32_t start = 0; start < ML_DSA_POLY_COEFFS_COUNT; start += 2 * len) {
			int32_t zeta = zetas[k++];

			for (uint32_t j = start; j < start + len; j++) {
				int32_t t = montgomery_reduce((int64_t)zeta * w[j + len]);

				/** Lazy reduction: |t| < q, so each of the 8 stages grows
				 *  the coefficient bound by at most q. With |input| < q the
				 *  output stays below 9q, well within int32.
				 */
				w[j + len] = w[j] - t;
				w[j] = w[j] + t;
			}
		}
	}
}

void cracen_ml_dsa_ntt_inversed(ml_dsa_poly_vector_t *vec)
{
	int32_t *w = vec->coeffs;
	uint8_t k = ML_DSA_POLY_COEFFS_COUNT - 1;
	/* (256^(-1) * R^2) mod q. The final Montgomery multiplication by this factor
	 * both applies the 1/256 scaling of the inverse NTT and cancels the R^(-1)
	 * factor that cracen_ml_dsa_multiply_ntt() leaves on every coefficient.
	 */
	const int32_t inv_256_r2_mont = 41978;

	/** The butterflies below skip modular reduction, which doubles the coefficient
	 *  bound per stage (256x overall), so bring the lazily-reduced input into
	 *  (-q, q) first: 256 * q still fits int32.
	 */
	for (uint32_t j = 0; j < ML_DSA_POLY_COEFFS_COUNT; j++) {
		w[j] = reduce32(w[j]);
	}

	for (uint32_t len = 1; len < ML_DSA_POLY_COEFFS_COUNT; len <<= 1) {
		for (uint32_t start = 0; start < ML_DSA_POLY_COEFFS_COUNT; start += 2 * len) {
			int32_t zeta = -zetas[k--];

			for (uint32_t j = start; j < start + len; j++) {
				int32_t t = w[j];

				w[j] = t + w[j + len];
				w[j + len] = t - w[j + len];
				w[j + len] = montgomery_reduce((int64_t)zeta * w[j + len]);
			}
		}
	}

	for (uint32_t j = 0; j < ML_DSA_POLY_COEFFS_COUNT; j++) {
		int32_t t = montgomery_reduce((int64_t)inv_256_r2_mont * w[j]);

		w[j] = t < 0 ? t + ML_DSA_PRIME_NUM : t;
	}
}

void cracen_ml_dsa_multiply_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b)
{
	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		/** Single Montgomery reduction: the result carries an extra R^(-1)
		 *  factor, which cracen_ml_dsa_ntt_inversed() cancels in its final
		 *  scaling. Valid while |a * b| < 2^31 * q, which the 9q output bound
		 *  of cracen_ml_dsa_ntt() satisfies (81q^2 < 2^31 * q).
		 */
		out_vec->coeffs[i] = montgomery_reduce((int64_t)a->coeffs[i] * b->coeffs[i]);
	}
}

void cracen_ml_dsa_add_ntt(ml_dsa_poly_vector_t *out_vec,
			   const ml_dsa_poly_vector_t *a,
			   const ml_dsa_poly_vector_t *b)
{
	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = a->coeffs[i] + b->coeffs[i];
	}
}

void cracen_ml_dsa_subtract_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b)
{
	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = a->coeffs[i] - b->coeffs[i];
	}
}
