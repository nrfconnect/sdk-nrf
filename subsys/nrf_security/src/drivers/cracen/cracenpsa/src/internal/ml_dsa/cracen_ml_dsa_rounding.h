/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA rounding and hints (FIPS 204).
 *
 * Decompose and MakeHint are defined here as static inline functions: they are called
 * once per coefficient from the innermost loops of signing and verification, where the
 * call overhead - and, for Decompose, the stack traffic needed to return its two outputs
 * through pointers - dominates their actual arithmetic.
 */

#ifndef CRACEN_ML_DSA_ROUNDING_H
#define CRACEN_ML_DSA_ROUNDING_H

#include <stdint.h>

#include "cracen_ml_dsa_internal.h"

/**
 * @brief Check if the provided mask is zero.
 *
 * @param[in] v Mask to check.
 *
 * @return All-ones mask when v == 0, zero otherwise.
 */
static inline int32_t ml_dsa_is_zero_mask(int32_t v)
{
	uint32_t x = (uint32_t)v;

	return (int32_t)(((x | (~x + 1u)) >> 31) - 1u);
}

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

/** FIPS 204, Algorithm 36 (Decompose), for r already reduced into [0, q).
 *  Splits r into high bits r1 and the centered low bits r0 in range (-gamma2, gamma2].
 *  Note: rounding range is gamma2 in terms of the spec.
 *
 *  r is secret-dependent, so this function avoids data-dependent division/modulo
 *  and branching on r: it uses the constant-time approximation inspired by the CRYSTALS-Dilithium
 *  reference implementation, which only branches on gamma2 (a public parameter with
 *  exactly two possible values).
 */

/**
 * @brief Decompose a coefficient into its high and low bits around 2*gamma2
 *        (FIPS 204, Algorithm 36 - Decompose).
 *
 * Splits r into high bits r1 and the centered low bits r0 in range (-gamma2, gamma2];
 * rounding range is gamma2 in terms of the spec.
 *
 * r is secret-dependent, so this function avoids data-dependent division/modulo
 * and branching on r: it uses the constant-time approximation inspired by the CRYSTALS-Dilithium
 * reference implementation, which only branches on gamma2 (a public parameter with
 * exactly two possible values).
 *
 * @note This function is for @p r already reduced into [0, q).
 *
 * @param[in] r       Coefficient in the range (-q, 2q); reduced mod q internally.
 * @param[in] gamma2  Low-order rounding range of the active parameter set.
 * @param[out] r0     The centered low-order bits of the coefficient.
 * @param[out] r1     The high-order bits of the coefficient.
 */
static inline void ml_dsa_decompose_reduced(int32_t r, uint32_t gamma2, int32_t *r0, int32_t *r1)
{
	int32_t alpha = (int32_t)(gamma2 << 1); /* alpha = 2*gamma2, see FIPS 204, Section 2.3 */
	int32_t high;
	int32_t low;

	high = (r + 127) >> 7;
	/** Note: Branching on gamma2, which is a public parameter with exactly two possible values
	 *  (depends on algorithm type).
	 */
	if (gamma2 == ML_DSA_GAMMA2(32)) {
		high = (high * 1025 + (1 << 21)) >> 22;
		high &= 15;
	} else {
		high = (high * 11275 + (1 << 23)) >> 24;
		high ^= ((43 - high) >> 31) & high;
	}

	low = r - high * alpha;
	low -= (((ML_DSA_PRIME_NUM - 1) / 2 - low) >> 31) & ML_DSA_PRIME_NUM;

	*r0 = low;
	*r1 = high;
}

/**
 * @brief Bring a coefficient back into [0, q) range,
 *        as required by ml_dsa_decompose_reduced().
 *        Signing forms these coefficients as sums and differences
 *        of inverse-NTT outputs (each already in [0, q)).
 *
 * @param[in] r Coefficient in the range (-q, 2q).
 *
 * @return Coefficient @p r in the range [0, q).
 */
static inline int32_t ml_dsa_center_to_zq(int32_t r)
{
	int32_t sum = ((r >> 31) & ML_DSA_PRIME_NUM) |
		       (((ML_DSA_PRIME_NUM - r - 1) >> 31) & -ML_DSA_PRIME_NUM);

	return r + sum;
}

/**
 * @brief Decompose a coefficient into its high and low bits around 2*gamma2
 *	  (FIPS 204, Algorithm 36 - Decompose).
 *
 * @param[in] r       Coefficient in the range (-q, 2q); reduced mod q internally.
 * @param[in] gamma2  Low-order rounding range of the active parameter set.
 * @param[out] r0     The centered low-order bits of the coefficient.
 * @param[out] r1     The high-order bits of the coefficient.
 */
static inline void cracen_ml_dsa_decompose(int32_t r, uint32_t gamma2, int32_t *r0, int32_t *r1)
{
	ml_dsa_decompose_reduced(ml_dsa_center_to_zq(r), gamma2, r0, r1);
}

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
static inline int32_t cracen_ml_dsa_make_hint(int32_t r0, int32_t r1, uint32_t gamma2)
{
	int32_t bound = (int32_t)gamma2;
	int32_t above_bound = (bound - r0) >> 31;
	int32_t below_bound = (r0 + bound) >> 31;
	/* All-ones if (r0 == -gamma2 && r1 != 0). */
	int32_t at_low_edge = ml_dsa_is_zero_mask(r0 + bound) & ~ml_dsa_is_zero_mask(r1);

	return (above_bound | below_bound | at_low_edge) & 1;
}

#endif /* CRACEN_ML_DSA_ROUNDING_H */
