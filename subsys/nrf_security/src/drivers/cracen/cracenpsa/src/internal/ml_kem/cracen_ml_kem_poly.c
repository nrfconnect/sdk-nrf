/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_kem_internal.h"
#include "cracen_ml_kem_poly.h"

/* q^(-1) mod 2^16, used by the Montgomery reduction. The value is 62209, given
 * here as its int16_t representative.
 */
#define ML_KEM_QINV	  (-3327)
/* round(2^26 / q), used by the Barrett reduction */
#define ML_KEM_BARRETT_V  20159
/* (128^(-1) * R^2) mod q, the final scaling of the inverse NTT */
#define ML_KEM_INV_128_R2 1441
/* R^2 mod q = 2^32 mod q, converts a plain value into the Montgomery domain */
#define ML_KEM_R2	  1353

/* zeta[k] * R mod q, i.e. zetas[k] = 17^brv(k) * 2^16 mod q, with the signed
 * representative closest to 0 (FIPS 203, Section 4.3).
 * Pre-scaling by R lets the transforms multiply by a zeta with
 * a single montgomery_reduce() call.
 */
static const int16_t zetas[ML_KEM_POLY_COEFFS_COUNT / 2] = {
	-1044, -758, -359, -1517, 1493, 1422, 287, 202,
	-171, 622, 1577, 182, 962, -1202, -1474, 1468,
	573, -1325, 264, 383, -829, 1458, -1602, -130,
	-681, 1017, 732, 608, -1542, 411, -205, -1571,
	1223, 652, -552, 1015, -1293, 1491, -282, -1544,
	516, -8, -320, -666, -1618, -1162, 126, 1469,
	-853, -90, -271, 830, 107, -1421, -247, -951,
	-398, 961, -1508, -725, 448, -1065, 677, -1275,
	-1103, 430, 555, 843, -1251, 871, 1550, 105,
	422, 587, 177, -235, -291, -460, 1574, 1653,
	-246, 778, 1159, -147, -777, 1483, -602, 1119,
	-1590, 644, -872, 349, 418, 329, -156, -75,
	817, 1097, 603, 610, 1322, -1285, -1465, 384,
	-1215, -136, 1218, -1335, -874, 220, -1187, -1659,
	-1185, -1530, -1278, 794, -1510, -854, -870, 478,
	-108, -308, 996, 991, 958, -1460, 1522, 1628,
};

/* Computes a * R^(-1) mod q for |a| < 2^15 * q.
 * Result lies in (-q, q).
 */
static int16_t montgomery_reduce(int32_t a)
{
	int16_t t = (int16_t)a * ML_KEM_QINV;

	return (int16_t)((a - (int32_t)t * ML_KEM_PRIME_NUM) >> 16);
}

/* Reduces any int16_t to a representative in (-q, q). */
static int16_t barrett_reduce(int16_t a)
{
	int16_t t = (int16_t)(((int32_t)ML_KEM_BARRETT_V * a + (1 << 25)) >> 26);

	return (int16_t)(a - (int32_t)t * ML_KEM_PRIME_NUM);
}

void cracen_ml_kem_ntt(ml_kem_poly_t *poly)
{
	int16_t *w = poly->coeffs;
	uint8_t k = 1;

	/** Seven layers */
	for (uint32_t len = 128; len >= 2; len >>= 1) {
		for (uint32_t start = 0; start < ML_KEM_POLY_COEFFS_COUNT; start += 2 * len) {
			int16_t zeta = zetas[k++];

			for (uint32_t j = start; j < start + len; j++) {
				int16_t t = montgomery_reduce(zeta * w[j + len]);

				/** Lazy reduction: |t| < q, so each of the seven stages
				 *  grows the coefficient bound by at most q. With
				 *  |input| < q the output stays below 8q since NTT for ML-KEM
				 *  has seven layers: 8 * 3329 = 26632 < 32767
				 *  (so it won't overflow int16).
				 */
				w[j + len] = w[j] - t;
				w[j] = w[j] + t;
			}
		}
	}

	cracen_ml_kem_reduce(poly);
}

void cracen_ml_kem_ntt_inversed(ml_kem_poly_t *poly)
{
	int16_t *w = poly->coeffs;
	uint8_t k = (ML_KEM_POLY_COEFFS_COUNT / 2) - 1;

	for (uint32_t len = 2; len <= 128; len <<= 1) {
		for (uint32_t start = 0; start < ML_KEM_POLY_COEFFS_COUNT; start += 2 * len) {
			int16_t zeta = zetas[k--];

			for (uint32_t j = start; j < start + len; j++) {
				int16_t t = w[j];

				/** Reducing sums on every stage to avoid int16 overflow. */
				w[j] = barrett_reduce(t + w[j + len]);
				w[j + len] = w[j + len] - t;
				w[j + len] = montgomery_reduce(zeta * w[j + len]);
			}
		}
	}

	for (uint32_t j = 0; j < ML_KEM_POLY_COEFFS_COUNT; j++) {
		w[j] = montgomery_reduce(w[j] * ML_KEM_INV_128_R2);
	}
}

/* One degree-1 base multiplication in Z_q[X]/(X^2 - zeta)
 * (FIPS 203, Algorithm 12, BaseCaseMultiply).
 */
static void base_case_multiply(int16_t *out, const int16_t *a, const int16_t *b, int16_t zeta)
{
	int16_t c0;
	int16_t c1;

	c0 = montgomery_reduce(a[1] * b[1]);
	c0 = montgomery_reduce(c0 * zeta);
	c0 = c0 + montgomery_reduce(a[0] * b[0]);

	c1 = montgomery_reduce(a[0] * b[1]);
	c1 = c1 + montgomery_reduce(a[1] * b[0]);

	out[0] = c0;
	out[1] = c1;
}

void cracen_ml_kem_multiply_ntt(ml_kem_poly_t *out, const ml_kem_poly_t *a, const ml_kem_poly_t *b)
{
	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT / 4; i++) {
		int16_t zeta = zetas[(ML_KEM_POLY_COEFFS_COUNT / 4) + i];

		base_case_multiply(&out->coeffs[4 * i],
				   &a->coeffs[4 * i],
				   &b->coeffs[4 * i],
				   zeta);

		base_case_multiply(&out->coeffs[4 * i + 2],
				   &a->coeffs[4 * i + 2],
				   &b->coeffs[4 * i + 2],
				   -zeta);
	}
}

void cracen_ml_kem_add(ml_kem_poly_t *out, const ml_kem_poly_t *a, const ml_kem_poly_t *b)
{
	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		out->coeffs[i] = a->coeffs[i] + b->coeffs[i];
	}
}

void cracen_ml_kem_subtract(ml_kem_poly_t *out, const ml_kem_poly_t *a, const ml_kem_poly_t *b)
{
	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		out->coeffs[i] = a->coeffs[i] - b->coeffs[i];
	}
}

void cracen_ml_kem_reduce(ml_kem_poly_t *poly)
{
	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		poly->coeffs[i] = barrett_reduce(poly->coeffs[i]);
	}
}

void cracen_ml_kem_to_montgomery(ml_kem_poly_t *poly)
{
	for (uint32_t i = 0; i < ML_KEM_POLY_COEFFS_COUNT; i++) {
		poly->coeffs[i] = montgomery_reduce(poly->coeffs[i] * ML_KEM_R2);
	}
}
