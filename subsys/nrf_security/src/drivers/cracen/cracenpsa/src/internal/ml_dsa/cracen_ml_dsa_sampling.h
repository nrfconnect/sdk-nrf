/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal definitions for the CRACEN ML-DSA pseudorandom sampling (FIPS 204).
 *
 */

#ifndef CRACEN_ML_DSA_SAMPLING_H
#define CRACEN_ML_DSA_SAMPLING_H

#include <psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Sample an NTT-domain polynomial by rejection from a 34-byte seed
 *	  (FIPS 204, Algorithm 30 - RejNTTPoly).
 *
 * @param[in] seed 34-byte seed used to expand the polynomial.
 * @param[out] out Output polynomial in the NTT domain.
 *
 * @return PSA_SUCCESS on success, or an error status otherwise.
 */
psa_status_t cracen_ml_dsa_rej_ntt_poly(const uint8_t *seed, ml_dsa_poly_vector_t *out);

/**
 * @brief Sample a polynomial with coefficients {-1, 0, 1} and a Hamming weight <= 64
 *	  (FIPS 204, Algorithm 29 - SampleInBall).
 *
 * @param[in] seed           Seed used to derive the sampled polynomial.
 * @param[in] seed_len       Length of @p seed in bytes.
 * @param[in] hamming_weight Number of nonzero (+-1) coefficients to place.
 * @param[out] out_vec       Output polynomial with the requested Hamming weight.
 *
 * @return PSA_SUCCESS on success, or an error status otherwise.
 */
psa_status_t cracen_ml_dsa_sample_in_ball(const uint8_t *seed, size_t seed_len,
					  uint8_t hamming_weight,
					  ml_dsa_poly_vector_t *out_vec);

#endif /* CRACEN_ML_DSA_SAMPLING_H */
