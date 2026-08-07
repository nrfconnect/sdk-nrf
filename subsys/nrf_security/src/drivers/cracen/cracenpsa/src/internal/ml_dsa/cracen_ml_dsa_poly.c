/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "cracen_ml_dsa_internal.h"
#include "cracen_ml_dsa_poly.h"

/* Reduce an arbitrary signed value into [0, q). */
static int32_t reduce_mod_q(int64_t a)
{
	a %= ML_DSA_PRIME_NUM;
	if (a < 0) {
		a += ML_DSA_PRIME_NUM;
	}
	return (int32_t)a;
}

/* Multiply two coefficients in [0, q); result in [0, q). */
static int32_t mul_mod(int32_t a, int32_t b)
{
	return (int32_t)(((int64_t)a * (int64_t)b) % ML_DSA_PRIME_NUM);
}

/* Modular exponentiation base^exp mod q, with base in [0, q). */
static int32_t pow_mod(int32_t base, uint32_t exp)
{
	int64_t result = 1;
	int64_t b = base;

	while (exp > 0) {
		if (exp & 1u) {
			result = (result * b) % ML_DSA_PRIME_NUM;
		}
		b = (b * b) % ML_DSA_PRIME_NUM;
		exp >>= 1;
	}

	return (int32_t)result;
}

/* Reverse the low 8 bits of m (the brv function from FIPS 204, Section 7.5). */
static uint8_t bit_reverse_8(uint8_t m)
{
	uint8_t r = 0;

	for (uint32_t i = 0; i < 8; i++) {
		r = (r << 1) | (m & 1u);
		m >>= 1;
	}

	return r;
}

void cracen_ml_dsa_ntt(ml_dsa_poly_vector_t *vec)
{
	int32_t *w = vec->coeffs;
	uint8_t k = 1;

	for (uint32_t len = 128; len >= 1; len >>= 1) {
		for (uint32_t start = 0; start < ML_DSA_POLY_COEFFS_COUNT; start += 2 * len) {
			int32_t zeta = pow_mod(ML_DSA_ROOT_OF_UNITY, bit_reverse_8(k++));

			for (uint32_t j = start; j < start + len; j++) {
				int32_t t = mul_mod(zeta, w[j + len]);

				w[j + len] = reduce_mod_q((int64_t)w[j] - t);
				w[j] = reduce_mod_q((int64_t)w[j] + t);
			}
		}
	}
}

void cracen_ml_dsa_ntt_inversed(ml_dsa_poly_vector_t *vec)
{
	int32_t *w = vec->coeffs;
	uint8_t k = ML_DSA_POLY_COEFFS_COUNT - 1;
	const int32_t inv_256_mod_q = 8347681; /* 256^(-1) mod q */
	int32_t zeta;

	for (uint32_t len = 1; len < ML_DSA_POLY_COEFFS_COUNT; len <<= 1) {
		for (uint32_t start = 0; start < ML_DSA_POLY_COEFFS_COUNT; start += 2 * len) {
			zeta = reduce_mod_q(-(int64_t)pow_mod(ML_DSA_ROOT_OF_UNITY,
							      bit_reverse_8(k--)));

			for (uint32_t j = start; j < start + len; j++) {
				int32_t t = w[j];

				w[j] = reduce_mod_q((int64_t)t + w[j + len]);
				w[j + len] = reduce_mod_q((int64_t)t - w[j + len]);
				w[j + len] = mul_mod(zeta, w[j + len]);
			}
		}
	}

	for (uint32_t j = 0; j < ML_DSA_POLY_COEFFS_COUNT; j++) {
		w[j] = mul_mod(inv_256_mod_q, w[j]);
	}
}

void cracen_ml_dsa_multiply_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b)
{
	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = mul_mod(a->coeffs[i], b->coeffs[i]);
	}
}

void cracen_ml_dsa_add_ntt(ml_dsa_poly_vector_t *out_vec,
			   const ml_dsa_poly_vector_t *a,
			   const ml_dsa_poly_vector_t *b)
{
	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = reduce_mod_q((int64_t)a->coeffs[i] + b->coeffs[i]);
	}
}

void cracen_ml_dsa_subtract_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b)
{
	for (uint32_t i = 0; i < ML_DSA_POLY_COEFFS_COUNT; i++) {
		out_vec->coeffs[i] = reduce_mod_q((int64_t)a->coeffs[i] - b->coeffs[i]);
	}
}
