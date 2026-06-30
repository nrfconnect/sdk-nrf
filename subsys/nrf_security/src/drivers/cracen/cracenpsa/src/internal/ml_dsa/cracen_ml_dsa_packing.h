/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA encoding/decoding (FIPS 204).
 *
 */

#ifndef CRACEN_ML_DSA_PACKING_H
#define CRACEN_ML_DSA_PACKING_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Pack one polynomial with coefficients in [0, b] into a byte string
 *	  (FIPS 204, Algorithm 16 - SimpleBitPack).
 *
 * @param[in] w        Polynomial whose coefficients are in the range [0, b].
 * @param[in] b        Upper bound on the coefficient values.
 * @param[out] out_vec Output byte string holding the packed polynomial.
 */
void cracen_ml_dsa_simple_bit_pack(const ml_dsa_poly_vector_t *w, uint32_t b,
				   uint8_t *out_vec);

/**
 * @brief Unpack one polynomial with coefficients in [0, b] from a byte string
 *	  (FIPS 204, Algorithm 18 - SimpleBitUnpack).
 *
 * @param[in] v        Byte string holding the packed polynomial.
 * @param[in] b        Upper bound on the coefficient values.
 * @param[out] out_vec Output polynomial with coefficients in the range [0, b].
 */
void cracen_ml_dsa_simple_bit_unpack(const uint8_t *v, uint32_t b,
				     ml_dsa_poly_vector_t *out_vec);

/**
 * @brief Unpack one polynomial with coefficients in [-a, b] from a byte string
 *	  (FIPS 204, Algorithm 19 - BitUnpack).
 *
 * @param[in] v        Byte string holding the packed polynomial.
 * @param[in] a        Lower bound (magnitude) on the coefficient values.
 * @param[in] b        Upper bound on the coefficient values.
 * @param[out] out_vec Output polynomial with coefficients in the range [-a, b].
 */
void cracen_ml_dsa_bit_unpack(const uint8_t *v, uint32_t a, uint32_t b,
			      ml_dsa_poly_vector_t *out_vec);

/**
 * @brief Decode the hint vector h from its encoded byte string
 *	  (FIPS 204, Algorithm 21 - HintBitUnpack).
 *
 * @param[in] alg_params Parameter set for the selected ML-DSA variant.
 * @param[in] y          Encoded hint byte string.
 * @param[out] h         Decoded hint vector.
 *
 * @return true on success, false on malformed input.
 */
bool cracen_ml_dsa_hint_bit_unpack(const ml_dsa_params_t *alg_params, const uint8_t *y, uint8_t *h);

#endif /* CRACEN_ML_DSA_PACKING_H */
