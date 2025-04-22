/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_RSA_ENCRYPTION_H
#define CRACEN_PSA_RSA_ENCRYPTION_H

#include <psa/crypto.h>

void cracen_rsa_oaep_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *pubkey,
			     struct cracen_crypt_text *text, struct sx_buf *label);

void cracen_rsa_oaep_decrypt(struct sitask *t, const struct sxhashalg *hashalg,
			     struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
			     struct sx_buf *label);

int cracen_rsa_pkcs1v15_decrypt(struct cracen_rsa_key *rsa_key, struct si_ase_text *text);

int cracen_rsa_pkcs1v15_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
				struct cracen_crypt_text *text, struct sx_buf *label);

#endif /* CRACEN_PSA_RSA_ENCRYPTION_H */
