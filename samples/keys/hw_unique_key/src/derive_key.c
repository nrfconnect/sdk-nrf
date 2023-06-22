/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <psa/crypto.h>
#include <hw_unique_key.h>

#include "derive_key.h"

#ifdef HUK_HAS_KMU
#define KEYSLOT HUK_KEYSLOT_MKEK
#else
#define KEYSLOT HUK_KEYSLOT_KDR
#endif


void hex_dump(uint8_t *buff, uint32_t len);

psa_key_id_t derive_key(psa_key_attributes_t *attributes, uint8_t *key_label,
			uint32_t label_size)
{
	uint8_t key_out[HUK_SIZE_BYTES];
	psa_key_id_t key_id_out = 0;
	psa_status_t status;

	int result = hw_unique_key_derive_key(KEYSLOT, NULL, 0,
		key_label, label_size, key_out, sizeof(key_out));
	if (result != HW_UNIQUE_KEY_SUCCESS) {
		printk("hw_unique_key_derive_key returned error: %d\n", result);
		return 0;
	}
	printk("Key:\n");
	hex_dump(key_out, sizeof(key_out));

	status = psa_import_key(attributes, key_out, sizeof(key_out), &key_id_out);
	if (status != PSA_SUCCESS) {
		printk("psa_import_key returned error: %d\n", status);
		return 0;
	}

	return key_id_out;
}
