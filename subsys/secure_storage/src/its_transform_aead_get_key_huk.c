/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/secure_storage/its/transform/aead_get.h>
#include <hw_unique_key.h>
#include <psa/crypto_values.h>

psa_status_t secure_storage_its_transform_aead_get_key(
		secure_storage_its_uid_t uid,
		uint8_t key[static CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE])
{
	int result;
	enum hw_unique_key_slot key_slot;

#ifdef HUK_HAS_KMU
	key_slot = HUK_KEYSLOT_MKEK;
#else
	key_slot = HUK_KEYSLOT_KDR;
#endif

	result = hw_unique_key_derive_key(key_slot, NULL, 0, (uint8_t *)&uid, sizeof(uid), key,
					  CONFIG_SECURE_STORAGE_ITS_TRANSFORM_AEAD_KEY_SIZE);
	if (result != HW_UNIQUE_KEY_SUCCESS) {
		return PSA_ERROR_BAD_STATE;
	}

	return PSA_SUCCESS;
}
