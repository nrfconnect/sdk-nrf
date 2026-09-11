/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal ML-DSA key expansion shared by signing and public-key derivation (FIPS 204).
 *
 * Not part of the public driver API.
 */

#ifndef CRACEN_ML_DSA_KEYGEN_H
#define CRACEN_ML_DSA_KEYGEN_H

#include <psa/crypto.h>
#include <stdint.h>

#include "cracen_ml_dsa_internal.h"

/** @brief Expand a key-pair seed into the ML-DSA secrets and public key
 *         (FIPS 204, Algorithm 6 - ML-DSA.KeyGen_internal).
 *
 * The matrix A is regenerated one cell at a time (ExpandA) and the public key is
 * encoded directly, so the private key is never materialized in the skEncode()
 * format. The three secret vectors are returned in the NTT domain, ready to be
 * consumed by the signing loop (FIPS 204, Algorithm 7, lines 2-4).
 *
 * @note 64-byte hash 𝑡𝑟 of the public key is not returned since it can easily
 *       be calculated as tr = H(pk, 64), as stated in FIPS 204, algorithm 6, line 9.
 *
 * @param[in] alg_params  Parameter set for the selected ML-DSA variant.
 * @param[in] seed        32-byte key-pair seed.
 * @param[out] k_secret   Signing secret K.
 * @param[out] s1_hat     Secret vector s1 in the NTT domain (columns l polynomials).
 * @param[out] s2_hat     Secret vector s2 in the NTT domain (rows k polynomials).
 * @param[out] t0_hat     Secret vector t0 in the NTT domain (rows k polynomials).
 * @param[out] pk         Encoded public key.
 *
 * @return PSA_SUCCESS on success, or an error status otherwise.
 */
psa_status_t cracen_ml_dsa_keygen_internal(const ml_dsa_params_t *alg_params, const uint8_t *seed,
					   uint8_t *k_secret, ml_dsa_poly_vector_t *s1_hat,
					   ml_dsa_poly_vector_t *s2_hat,
					   ml_dsa_poly_vector_t *t0_hat, uint8_t *pk);

#endif /* CRACEN_ML_DSA_KEYGEN_H */
