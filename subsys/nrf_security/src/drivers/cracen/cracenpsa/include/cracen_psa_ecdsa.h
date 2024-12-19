/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <psa/crypto.h>

int cracen_ecdsa_verify_message(const char *pubkey, const struct sxhashalg *hashalg,
				const uint8_t *message, size_t message_length,
				const struct sx_pk_ecurve *curve, const uint8_t *signature);

int cracen_ecdsa_verify_digest(const char *pubkey, const uint8_t *digest, size_t digestsz,
			       const struct sx_pk_ecurve *curve, const uint8_t *signature);

int cracen_ecdsa_sign_message(const struct ecc_priv_key *privkey, const struct sxhashalg *hashalg,
			      const struct sx_pk_ecurve *curve, const uint8_t *message,
			      size_t message_length, const uint8_t *signature);

int cracen_ecdsa_sign_digest(const struct ecc_priv_key *privkey, const struct sxhashalg *hashalg,
			     const struct sx_pk_ecurve *curve, const uint8_t *digest,
			     size_t digest_length, const uint8_t *signature);

int cracen_ecdsa_sign_message_deterministic(const struct ecc_priv_key *privkey,
					    const struct sxhashalg *hashalg,
					    const struct sx_pk_ecurve *curve,
					    const uint8_t *message, size_t message_length,
					    const uint8_t *signature);

int cracen_ecdsa_sign_digest_deterministic(const struct ecc_priv_key *privkey,
					   const struct sxhashalg *hashalg,
					   const struct sx_pk_ecurve *curve, const uint8_t *digest,
					   size_t digestsz, const uint8_t *signature);
