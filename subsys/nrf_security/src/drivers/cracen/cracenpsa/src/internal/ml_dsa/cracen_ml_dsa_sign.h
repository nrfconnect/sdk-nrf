/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Internal ML-DSA signing primitive (FIPS 204, Algorithm 7 - ML-DSA.Sign_internal).
 *
 * Not part of the public driver API.
 */

#ifndef CRACEN_ML_DSA_SIGN_H
#define CRACEN_ML_DSA_SIGN_H

#include <psa/crypto.h>
#include <stddef.h>
#include <stdint.h>

#include "cracen_ml_dsa_internal.h"

/** @brief Produce an ML-DSA signature from the internal message representative components
 *         (FIPS 204, Algorithm 7 - ML-DSA.Sign_internal).
 *
 * The message representative mu is built from the M' components (domain, ctx, oid, msg), so the
 * same routine serves both pure ML-DSA and HashML-DSA. The private key is expanded on demand
 * from the 32-byte key-pair seed.
 *
 * @param[in] alg_params  Parameter set for the selected ML-DSA variant.
 * @param[in] seed        32-byte key-pair seed.
 * @param[in] rnd         Per-signature randomizer (32 bytes); all-zero for the
 *                        deterministic variants.
 * @param[in] domain      0 for pure ML-DSA, 1 for HashML-DSA.
 * @param[in] ctx         Context string (may be NULL when empty).
 * @param[in] ctx_len     Length of @p ctx in bytes.
 * @param[in] oid         Pre-hash function DER OID (HashML-DSA only), NULL for pure ML-DSA.
 * @param[in] oid_len     Length of @p oid in bytes (0 for pure ML-DSA).
 * @param[in] msg         Message, or the pre-hashed message PH(M) for HashML-DSA.
 * @param[in] msg_len     Length of @p msg in bytes.
 * @param[out] sig        Buffer receiving the encoded signature.
 *
 * @return PSA_SUCCESS on success, or an error status otherwise.
 */
psa_status_t cracen_ml_dsa_sign_internal(const ml_dsa_params_t *alg_params, const uint8_t *seed,
					 const uint8_t *rnd, uint8_t domain, const uint8_t *ctx,
					 size_t ctx_len, const uint8_t *oid, size_t oid_len,
					 const uint8_t *msg, size_t msg_len, uint8_t *sig);

#endif /* CRACEN_ML_DSA_SIGN_H */
