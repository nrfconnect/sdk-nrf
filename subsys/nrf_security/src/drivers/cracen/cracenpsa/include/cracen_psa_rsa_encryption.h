/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef CRACEN_PSA_RSA_ENCRYPTION_H
#define CRACEN_PSA_RSA_ENCRYPTION_H

#include "cracen_psa_primitives.h"

int cracen_rsa_oaep_encrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length);

int cracen_rsa_oaep_decrypt(const struct sxhashalg *hashalg, struct cracen_rsa_key *rsa_key,
			    struct cracen_crypt_text *text, struct sx_const_buf *label,
			    uint8_t *output, size_t *output_length);

int cracen_rsa_pkcs1v15_decrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length);

int cracen_rsa_pkcs1v15_encrypt(struct cracen_rsa_key *rsa_key, struct cracen_crypt_text *text,
				uint8_t *output, size_t *output_length);

#endif /* CRACEN_PSA_RSA_ENCRYPTION_H */
