/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Polynomial arithmetic of the CRACEN ML-KEM (FIPS 203) implementation.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 *
 * @details
 * Similarly to the ML-DSA implementation, NTT-domain values use lazy reduction.
 * This means that every multiplication is a single multiply-and-reduce,
 * and the R^(-1) factor it introduces is cancelled either by the final scaling
 * of the inverse NTT or by cracen_ml_kem_to_montgomery().
 * The cancellation by cracen_ml_kem_to_montgomery() is used
 * for values that stay in the NTT domain.
 *
 * However, Montgomery domain uses R = 2^16, which is different from ML-DSA.
 */

#ifndef CRACEN_ML_KEM_POLY_H
#define CRACEN_ML_KEM_POLY_H

#include "cracen_ml_kem_internal.h"

/** @brief Compute ̂the NTT representation of a polynomial, in place
 *         (FIPS 203, Algorithm 9).
 *
 * @param[in,out] poly  Polynomial with coefficients in (-q, q). On return the
 *                      coefficients are the NTT representation, reduced to
 *                      (-q, q).
 */
void cracen_ml_kem_ntt(ml_kem_poly_t *poly);

/** @brief Perform inverse number-theoretic transform, in place (FIPS 203, Algorithm 10).
 *
 * The final scaling by 128^(-1) * R^2 also cancels the R^(-1) factor that
 * cracen_ml_kem_multiply_ntt() leaves behind. Consequently, a product formed in the NTT
 * domain and transformed back needs no further correction using cracen_ml_kem_to_montgomery().
 *
 * @param[in,out] poly  Polynomial in the NTT domain.
 */
void cracen_ml_kem_ntt_inversed(ml_kem_poly_t *poly);

/** @brief Multiply two polynomials in the NTT domain (FIPS 203, Algorithm 11).
 *
 * The result carries an extra R^(-1) factor. See cracen_ml_kem_ntt_inversed()
 * and cracen_ml_kem_to_montgomery().
 *
 * @param[out] out  Product. May be the same value as @p a or @p b.
 * @param[in] a     First operand in the NTT domain.
 * @param[in] b     Second operand in the NTT domain.
 */
void cracen_ml_kem_multiply_ntt(ml_kem_poly_t *out, const ml_kem_poly_t *a,
				const ml_kem_poly_t *b);

/** @brief Add two polynomials coefficient-wise, without reduction.
 *
 * @param[out] out  Sum. May be the same value as @p a or @p b.
 * @param[in] a     First operand.
 * @param[in] b     Second operand.
 */
void cracen_ml_kem_add(ml_kem_poly_t *out, const ml_kem_poly_t *a, const ml_kem_poly_t *b);

/** @brief Subtract two polynomials coefficient-wise, without reduction.
 *
 * @param[out] out  Difference @p a - @p b. May be the same value as @p a or @p b.
 * @param[in] a     Minuend.
 * @param[in] b     Subtrahend.
 */
void cracen_ml_kem_subtract(ml_kem_poly_t *out, const ml_kem_poly_t *a, const ml_kem_poly_t *b);

/** @brief Reduce every coefficient to (-q, q), in place.
 *
 * @param[in,out] poly  Polynomial to reduce.
 */
void cracen_ml_kem_reduce(ml_kem_poly_t *poly);

/** @brief Multiply every coefficient by R, in place.
 *
 * Applied to a product that stays in the NTT domain, this cancels the R^(-1)
 * factor of cracen_ml_kem_multiply_ntt().
 *
 * @param[in,out] poly  Polynomial to convert.
 */
void cracen_ml_kem_to_montgomery(ml_kem_poly_t *poly);

#endif /* CRACEN_ML_KEM_POLY_H */
