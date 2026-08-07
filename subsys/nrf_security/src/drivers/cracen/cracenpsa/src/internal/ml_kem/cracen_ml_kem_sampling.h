/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Sampling of the CRACEN ML-KEM (FIPS 203) implementation.
 *
 * @note These APIs are for internal use only. Applications must use the
 *          PSA Crypto API (psa_* functions) instead of calling these functions
 *          directly.
 */

#ifndef CRACEN_ML_KEM_SAMPLING_H
#define CRACEN_ML_KEM_SAMPLING_H

#include "cracen_ml_kem_internal.h"

#include <psa/crypto_types.h>

/** @brief Convert a seed together with two indexing bytes into a polynomial
 *         in the NTT domain (FIPS 203, Algorithm 7, SampleNTT).
 *
 * The two index bytes are appended to @p rho in the order the caller supplies them.
 *
 * @param[in] rho    32-byte public seed.
 * @param[in] index0 First index byte.
 * @param[in] index1 Second index byte.
 * @param[out] out   Sampled polynomial; coefficients in [0, q).
 *
 * @retval PSA_SUCCESS  The operation completed successfully.
 */
psa_status_t cracen_ml_kem_sample_ntt(const uint8_t *rho, uint8_t index0, uint8_t index1,
				      ml_kem_poly_t *out);

/** @brief Take a seed as input and output a pseudorandom sample
 *         from a centered binomial distribution (FIPS 203, Algorithm 8, SamplePolyCBD).
 *
 *         Across the FIPS203 standard, this algorithm always uses a result of PRF_eta(seed, nonce)
 *         (pseudorandom function) as input, so its calculation is done here as well.
 *
 * @param[in] seed   32-byte secret seed (sigma or the encapsulation randomness).
 * @param[in] nonce  Domain separation byte; N in the specification.
 * @param[in] eta    Distribution parameter; eta1 or eta2.
 * @param[out] out   Sampled polynomial; coefficients in [-eta, eta].
 *
 * @retval PSA_SUCCESS  The operation completed successfully.
 */
psa_status_t cracen_ml_kem_sample_poly_cbd(const uint8_t *seed, uint8_t nonce, uint8_t eta,
					   ml_kem_poly_t *out);

#endif /* CRACEN_ML_KEM_SAMPLING_H */
