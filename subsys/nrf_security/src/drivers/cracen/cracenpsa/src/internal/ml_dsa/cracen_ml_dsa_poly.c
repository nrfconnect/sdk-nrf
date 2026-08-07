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
static int32_t zetas[] = {
	0, 25847, 5771523, 7861508, 237124, 7602457, 7504169, 466468,
	1826347, 2353451, 8021166, 6288512, 3119733, 5495562, 3111497, 2680103,
	2725464, 1024112, 7300517, 3585928, 7830929, 7260833, 2619752, 6271868,
	6262231, 4520680, 6980856, 5102745, 1757237, 8360995, 4010497, 280005,
	2706023, 95776, 3077325, 3530437, 6718724, 4788269, 5842901, 3915439,
	4519302, 5336701, 3574422, 5512770, 3539968, 8079950, 2348700, 7841118,
	6681150, 6736599, 3505694, 4558682, 3507263, 6239768, 6779997, 3699596,
	811944, 531354, 954230, 3881043, 3900724, 5823537, 2071892, 5582638,
	4450022, 6851714, 4702672, 5339162, 6927966, 3475950, 2176455, 6795196,
	7122806, 1939314, 4296819, 7380215, 5190273, 5223087, 4747489, 126922,
	3412210, 7396998, 2147896, 2715295, 5412772, 4686924, 7969390, 5903370,
	7709315, 7151892, 8357436, 7072248, 7998430, 1349076, 1852771, 6949987,
	5037034, 264944, 508951, 3097992, 44288, 7280319, 904516, 3958618,
	4656075, 8371839, 1653064, 5130689, 2389356, 8169440, 759969, 7063561,
	189548, 4827145, 3159746, 6529015, 5971092, 8202977, 1315589, 1341330,
	1285669, 6795489, 7567685, 6940675, 5361315, 4499357, 4751448, 3839961,
	2091667, 3407706, 2316500, 3817976, 5037939, 2244091, 5933984, 4817955,
	266997, 2434439, 7144689, 3513181, 4860065, 4621053, 7183191, 5187039,
	900702, 1859098, 909542, 819034, 495491, 6767243, 8337157, 7857917,
	7725090, 5257975, 2031748, 3207046, 4823422, 7855319, 7611795, 4784579,
	342297, 286988, 5942594, 4108315, 3437287, 5038140, 1735879, 203044,
	2842341, 2691481, 5790267, 1265009, 4055324, 1247620, 2486353, 1595974,
	4613401, 1250494, 2635921, 4832145, 5386378, 1869119, 1903435, 7329447,
	7047359, 1237275, 5062207, 6950192, 7929317, 1312455, 3306115, 6417775,
	7100756, 1917081, 5834105, 7005614, 1500165, 777191, 2235880, 3406031,
	7838005, 5548557, 6709241, 6533464, 5796124, 4656147, 594136, 4603424,
	6366809, 2432395, 2454455, 8215696, 1957272, 3369112, 185531, 7173032,
	5196991, 162844, 1616392, 3014001, 810149, 1652634, 4686184, 6581310,
	5341501, 3523897, 3866901, 269760, 2213111, 7404533, 1717735, 472078,
	7953734, 1723600, 6577327, 1910376, 6712985, 7276084, 8119771, 4546524,
	5441381, 6144432, 7959518, 6094090, 183443, 7403526, 1612842, 4834730,
	7826001, 3919660, 8332111, 7018208, 3937738, 1400424, 7534263, 1976782
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
