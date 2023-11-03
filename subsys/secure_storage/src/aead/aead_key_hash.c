/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <stdio.h>

#include "aead_key.h"

psa_status_t secure_storage_get_key(psa_storage_uid_t uid, uint8_t *key_buf, size_t key_length)
{

	size_t olen;

	if (key_length < AEAD_KEY_SIZE) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	return psa_hash_compute(PSA_ALG_SHA_256, (uint8_t *)&uid, sizeof(uid), key_buf, key_length,
				&olen);
}
