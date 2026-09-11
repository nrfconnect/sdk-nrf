/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal ML-DSA verification primitive (FIPS 204, Algorithm 8 - ML-DSA.Verify_internal).
 *
 * Not part of the public driver API.
 */

#ifndef CRACEN_ML_DSA_VERIFY_H
#define CRACEN_ML_DSA_VERIFY_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>

#include "cracen_ml_dsa_internal.h"

/** @brief Verify an ML-DSA signature from the internal message representative components
 *         (FIPS 204, Algorithm 8 - ML-DSA.Verify_internal).
 *
 * The message representative mu is built from the M' components (domain, ctx, oid, msg), so the
 * same routine serves both pure ML-DSA and HashML-DSA.
 *
 * @param[in] alg_params  Parameter set for the selected ML-DSA variant.
 * @param[in] pk          Encoded public key.
 * @param[in] sig         Signature to verify.
 * @param[in] domain      0 for pure ML-DSA, 1 for HashML-DSA.
 * @param[in] ctx         Context string (may be NULL when empty).
 * @param[in] ctx_len     Length of @p ctx in bytes.
 * @param[in] oid         Pre-hash function DER OID (HashML-DSA only), NULL for pure ML-DSA.
 * @param[in] oid_len     Length of @p oid in bytes (0 for pure ML-DSA).
 * @param[in] msg         Message, or the pre-hashed message PH(M) for HashML-DSA.
 * @param[in] msg_len     Length of @p msg in bytes.
 *
 * @retval PSA_SUCCESS                 The signature is valid.
 * @retval PSA_ERROR_INVALID_SIGNATURE The signature is malformed or invalid.
 *
 * @return Another error status if the verification could not be completed.
 */
psa_status_t cracen_ml_dsa_verify_internal(const ml_dsa_params_t *alg_params, const uint8_t *pk,
					   const uint8_t *sig, uint8_t domain, const uint8_t *ctx,
					   size_t ctx_len, const uint8_t *oid, size_t oid_len,
					   const uint8_t *msg, size_t msg_len);

#endif /* CRACEN_ML_DSA_VERIFY_H */
