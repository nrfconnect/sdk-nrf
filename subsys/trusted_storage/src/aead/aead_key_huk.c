/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <hw_unique_key.h>
#include <zephyr/init.h>

#include "aead_key.h"

#ifdef HUK_HAS_KMU
#define KEYSLOT HUK_KEYSLOT_MKEK
#else
#define KEYSLOT HUK_KEYSLOT_KDR
#endif

psa_status_t trusted_storage_get_key(psa_storage_uid_t uid, uint8_t *key_buf, size_t key_length)
{

	int result;

	if (key_length < AEAD_KEY_SIZE) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (!hw_unique_key_are_any_written()) {
		return PSA_ERROR_BAD_STATE;
	}

	result = hw_unique_key_derive_key(KEYSLOT, NULL, 0, (uint8_t *)&uid, sizeof(uid), key_buf,
					  key_length);
	if (result != HW_UNIQUE_KEY_SUCCESS) {
		return PSA_ERROR_BAD_STATE;
	}

	return PSA_SUCCESS;
}
