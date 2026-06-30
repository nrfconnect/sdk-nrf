/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA polynomial arithmetic (FIPS 204).
 *	  All polynomials are in the range [0, q); NTT-domain operations are pointwise.
 *
 */

#ifndef CRACEN_ML_DSA_POLY_H
#define CRACEN_ML_DSA_POLY_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Compute the forward number-theoretic transform (NTT) of a polynomial in place
 *	  (FIPS 204, Algorithm 41 - NTT).
 *
 * @param[in,out] vec Polynomial transformed into the NTT domain in place.
 */
void cracen_ml_dsa_ntt(ml_dsa_poly_vector_t *vec);

/**
 * @brief Compute the inverse number-theoretic transform of a polynomial in place
 *	  (FIPS 204, Algorithm 42 - NTT^{-1}).
 *
 * @param[in,out] vec Polynomial transformed out of the NTT domain in place.
 */
void cracen_ml_dsa_ntt_inversed(ml_dsa_poly_vector_t *vec);

/**
 * @brief Compute the pointwise product of two NTT-domain polynomials modulo q
 *	  (FIPS 204, Algorithm 45 - MultiplyNTT).
 *
 * @param[out] out_vec Output polynomial holding out_vec = a * b mod q.
 * @param[in] a        First NTT-domain operand.
 * @param[in] b        Second NTT-domain operand.
 */
void cracen_ml_dsa_multiply_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b);

/**
 * @brief Add two NTT-domain polynomials modulo q
 *	  (FIPS 204, Algorithm 44 - AddNTT).
 *
 * @param[out] out_vec Output polynomial holding out_vec = a + b mod q.
 * @param[in] a        First NTT-domain operand.
 * @param[in] b        Second NTT-domain operand.
 */
void cracen_ml_dsa_add_ntt(ml_dsa_poly_vector_t *out_vec,
			   const ml_dsa_poly_vector_t *a,
			   const ml_dsa_poly_vector_t *b);

/**
 * @brief Subtract two NTT-domain polynomials modulo q.
 *
 * @param[out] out_vec Output polynomial holding out_vec = a - b mod q.
 * @param[in] a        First NTT-domain operand.
 * @param[in] b        Second NTT-domain operand.
 */
void cracen_ml_dsa_subtract_ntt(ml_dsa_poly_vector_t *out_vec,
				const ml_dsa_poly_vector_t *a,
				const ml_dsa_poly_vector_t *b);

#endif /* CRACEN_ML_DSA_POLY_H */
