/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA polynomial arithmetic (FIPS 204).
 *
 * NTT-domain values use lazy reduction, as in the FIPS 204 reference implementation:
 * intermediate coefficients are only congruent mod q and each function documents the
 * coefficient bounds it requires and guarantees. cracen_ml_dsa_ntt_inversed() brings
 * coefficients back to the range [0, q).
 */

#ifndef CRACEN_ML_DSA_POLY_H
#define CRACEN_ML_DSA_POLY_H

#include "cracen_ml_dsa_internal.h"

/**
 * @brief Compute the forward number-theoretic transform (NTT) of a polynomial in place
 *	  (FIPS 204, Algorithm 41 - NTT).
 *
 * No modular reduction is performed after additions and subtractions (lazy reduction).
 * Input coefficients may use any signed representation with an absolute value below q;
 * output coefficients are congruent mod q with an absolute value below 9q.
 *
 * @param[in,out] vec Polynomial transformed into the NTT domain in place.
 */
void cracen_ml_dsa_ntt(ml_dsa_poly_vector_t *vec);

/**
 * @brief Compute the inverse number-theoretic transform of a polynomial in place
 *	  (FIPS 204, Algorithm 42 - NTT^{-1}).
 *
 * Accepts lazily-reduced input with |coefficient| <= 2^31 - 2^22 - 1. Every input
 * coefficient must carry the R^(-1) factor left by cracen_ml_dsa_multiply_ntt();
 * the final scaling cancels it. Output coefficients are in the range [0, q).
 *
 * @param[in,out] vec Polynomial transformed out of the NTT domain in place.
 */
void cracen_ml_dsa_ntt_inversed(ml_dsa_poly_vector_t *vec);

/**
 * @brief Compute the pointwise product of two NTT-domain polynomials modulo q
 *	  (FIPS 204, Algorithm 45 - MultiplyNTT).
 *
 * Uses a single Montgomery reduction per coefficient, so the result is
 * a * b * R^(-1) mod q (for R = 2^32) with |coefficient| < q; the extra R^(-1)
 * factor is canceled by cracen_ml_dsa_ntt_inversed(). Operand coefficient bounds
 * must satisfy |a * b| < 2^31 * q, which holds for any cracen_ml_dsa_ntt() output.
 *
 * @param[out] out_vec Output polynomial holding out_vec = a * b * R^(-1) mod q.
 * @param[in] a        First NTT-domain operand.
 * @param[in] b        Second NTT-domain operand.
 */
void cracen_ml_dsa_multiply_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b);

/**
 * @brief Add two NTT-domain polynomials
 *	  (FIPS 204, Algorithm 44 - AddNTT).
 *
 * No modular reduction is performed (lazy reduction): the caller must keep the
 * accumulated coefficient bound within the input limit of
 * cracen_ml_dsa_ntt_inversed().
 *
 * @param[out] out_vec Output polynomial holding out_vec = a + b.
 * @param[in] a        First NTT-domain operand.
 * @param[in] b        Second NTT-domain operand.
 */
void cracen_ml_dsa_add_ntt(ml_dsa_poly_vector_t *out_vec,
			   const ml_dsa_poly_vector_t *a,
			   const ml_dsa_poly_vector_t *b);

/**
 * @brief Subtract two NTT-domain polynomials.
 *
 * No modular reduction is performed (lazy reduction): the caller must keep the
 * accumulated coefficient bound within the input limit of
 * cracen_ml_dsa_ntt_inversed().
 *
 * @param[out] out_vec Output polynomial holding out_vec = a - b.
 * @param[in] a        First NTT-domain operand.
 * @param[in] b        Second NTT-domain operand.
 */
void cracen_ml_dsa_subtract_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b);

#endif /* CRACEN_ML_DSA_POLY_H */
