/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @addtogroup cracen_psa_hybrid_verify
 * @{
 * @brief Concurrent ML-DSA and ECDSA verification (internal use only).
 */

#ifndef CRACEN_PSA_HYBRID_VERIFY_H
#define CRACEN_PSA_HYBRID_VERIFY_H

#include <stddef.h>
#include <stdint.h>

#include <psa/crypto.h>

/** @brief ML-DSA component of a hybrid signature.
 *
 * @c alg is PSA_ALG_ML_DSA or PSA_ALG_DETERMINISTIC_ML_DSA. @c key_bits
 * selects the parameter set: 128 (ML-DSA-44), 192 (ML-DSA-65) or 256
 * (ML-DSA-87).
 */
struct cracen_hybrid_verify_ml_dsa {
	psa_algorithm_t alg;
	size_t key_bits;
	const uint8_t *public_key;
	size_t public_key_length;
	const uint8_t *signature;
	size_t signature_length;
};

/** @brief ECDSA component of a hybrid signature.
 *
 * @c alg is PSA_ALG_ECDSA(hash_alg) or PSA_ALG_DETERMINISTIC_ECDSA(hash_alg)
 * for the hash algorithm the signature was created with. The public key is
 * uncompressed, 0x04 || Qx || Qy, as exported by psa_export_public_key().
 */
struct cracen_hybrid_verify_ecdsa {
	psa_algorithm_t alg;
	psa_ecc_family_t family;
	size_t key_bits;
	const uint8_t *public_key;
	size_t public_key_length;
	const uint8_t *signature;
	size_t signature_length;
};

/** @brief Verify an ML-DSA and an ECDSA signature over the same message.
 *
 * Both signatures must cover @p message and both must be valid.
 *
 * @p message is read twice: once to compute the ECDSA digest and once
 * inside the ML-DSA verification, with the PKE running in between. The
 * caller must not modify it for the duration of the call, or the two
 * signatures can end up verifying different content while this still
 * reports success.
 *
 * Nothing binds the ECDSA and the ML-DSA public keys to each other. A
 * domain separator in @p message binds the message to a specific use, not
 * the two keys to a single identity; a caller that needs the keys bound
 * must do so itself, for example with a certificate or transcript that
 * names both.
 *
 * @retval ::PSA_SUCCESS Both signatures are valid.
 * @retval ::PSA_ERROR_INVALID_SIGNATURE At least one signature is invalid.
 * @retval Other PSA status codes on invalid arguments and internal errors. The
 *         ECDSA status is returned when both components fail.
 */
psa_status_t cracen_hybrid_verify(const uint8_t *message, size_t message_length,
				 const struct cracen_hybrid_verify_ml_dsa *ml_dsa,
				 const struct cracen_hybrid_verify_ecdsa *ecdsa);

/** @} */

#endif /* CRACEN_PSA_HYBRID_VERIFY_H */
