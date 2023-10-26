/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <psa/crypto.h>
#include <hw_unique_key.h>

#include "derive_key.h"

#ifdef HUK_HAS_KMU
#define KEYSLOT HUK_KEYSLOT_MKEK
#else
#define KEYSLOT HUK_KEYSLOT_KDR
#endif

#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})

LOG_MODULE_DECLARE(app, LOG_LEVEL_DBG);

psa_status_t derive_key(psa_key_attributes_t *attributes, uint8_t *key_label,
			uint32_t label_size, psa_key_id_t *key_id_out)
{
	uint8_t key_out[HUK_SIZE_BYTES];
	psa_key_id_t key_id;
	psa_status_t status;
	int err;

	*key_id_out = PSA_KEY_ID_NULL;

	err = hw_unique_key_derive_key(KEYSLOT, NULL, 0,
				       key_label, label_size,
				       key_out, sizeof(key_out));
	if (err != HW_UNIQUE_KEY_SUCCESS) {
		LOG_INF("hw_unique_key_derive_key returned error: %d", err);
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	PRINT_HEX("Key", key_out, sizeof(key_out));

	status = psa_import_key(attributes, key_out, sizeof(key_out), &key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_import_key returned error: %d", status);
		return status;
	}

	*key_id_out = key_id;
	return PSA_SUCCESS;
}
