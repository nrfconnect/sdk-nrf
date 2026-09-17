/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA rounding and hints (FIPS 204).
 *
 */

#ifndef CRACEN_ML_DSA_ROUNDING_H
#define CRACEN_ML_DSA_ROUNDING_H

#include <stdint.h>

#include "cracen_ml_dsa_internal.h"

/**
 * @brief Adjust the coefficient according to the provided hint
 *	  (FIPS 204, Algorithm 40 - UseHint).
 *
 * @param[in] hint_bit Hint bit (0 or 1) for the coefficient.
 * @param[in] r        Coefficient to which the hint is applied.
 * @param[in] gamma2   Low-order rounding range of the active parameter set.
 *
 * @return The high-order bits w1 of the coefficient after applying the hint.
 */
int32_t cracen_ml_dsa_use_hint(int32_t hint_bit, int32_t r, uint32_t gamma2);

/**
 * @brief Split every coefficient into its high and low bits around 2^d
 *	  (FIPS 204, Algorithm 35 - Power2Round).
 *
 * Each input coefficient r (in [0, q)) is written as r = r1 * 2^d + r0 with the
 * low part r0 in the centered range (-2^(d-1), 2^(d-1)].
 *
 * @param[in] in   Polynomial with coefficients in [0, q).
 * @param[out] t1  High-bits polynomial (r1 for each coefficient).
 * @param[out] t0  Low-bits polynomial (centered r0 for each coefficient).
 */
void cracen_ml_dsa_power2round(const ml_dsa_poly_vector_t *in,
			       ml_dsa_poly_vector_t *t1,
			       ml_dsa_poly_vector_t *t0);

/**
 * @brief Decompose a coefficient into its high and low bits around 2*gamma2
 *	  (FIPS 204, Algorithm 36 - Decompose).
 *
 * @param[in] r       Coefficient in the range (-q, 2q); reduced mod q internally.
 * @param[in] gamma2  Low-order rounding range of the active parameter set.
 * @param[out] r0     The centered low-order bits of the coefficient.
 * @param[out] r1     The high-order bits of the coefficient.
 */
void cracen_ml_dsa_decompose(int32_t r, uint32_t gamma2, int32_t *r0, int32_t *r1);

/**
 * @brief Return the high-order bits of a coefficient
 *	  (FIPS 204, Algorithm 37 - HighBits).
 *
 * @param[in] r      Coefficient in the range (-q, 2q); reduced mod q internally.
 * @param[in] gamma2 Low-order rounding range of the active parameter set.
 *
 * @return The high-order bits r1 of the coefficient.
 */
int32_t cracen_ml_dsa_high_bits(int32_t r, uint32_t gamma2);

/**
 * @brief Compute a hint bit for an already decomposed coefficient
 *	  (FIPS 204, Algorithm 39 - MakeHint).
 *
 * @note This function is implemented so as to use an alternative (and faster) way of computing
 *	 the hints specified in Section 5.1 of
 *	 https://pq-crystals.org/dilithium/data/dilithium-specification-round3-20210208.pdf.
 *
 * @param[in] r0     Low-order bits of r plus the perturbation z, requires |z| <= gamma2.
 * @param[in] r1     High-order bits of r.
 * @param[in] gamma2 Low-order rounding range of the active parameter set.
 *
 * @return 1 if adding z changes the high bits of r, 0 otherwise.
 */
int32_t cracen_ml_dsa_make_hint(int32_t r0, int32_t r1, uint32_t gamma2);

#endif /* CRACEN_ML_DSA_ROUNDING_H */
