/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "psa_core_lite_volatile_key_storage.h"
#include <psa/crypto.h>

#if defined(CONFIG_PSA_CRYPTO_DRIVER_CRACEN)
#include <cracen/mem_helpers.h>
#endif

#define PSA_LITE_TRUE	0xA5FFA5FF
#define PSA_LITE_FALSE	0u

typedef struct {
	psa_lite_key_slot_t slot;
	uint32_t occupied;
} psa_lite_key_slot_entry_t;

static psa_lite_key_slot_entry_t g_key_slots[PSA_LITE_MAX_KEYS_SUPPORTED] = {};

void psa_lite_free_key_slot(mbedtls_svc_key_id_t key_id)
{
	if (!psa_lite_key_id_is_volatile(key_id)) {
		return;
	}

	safe_memzero(&g_key_slots[key_id - PSA_LITE_KEY_ID_MIN].slot, sizeof(psa_lite_key_slot_t));
	g_key_slots[key_id - PSA_LITE_KEY_ID_MIN].occupied = PSA_LITE_FALSE;
}

psa_status_t psa_lite_get_key_slot(mbedtls_svc_key_id_t *key_id, psa_lite_key_slot_t **slot)
{
	/* Note: it is assumed that key_id has already been verified for volatile key */
	if (key_id == NULL || slot == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (*key_id != PSA_LITE_KEY_ID_NULL) {
		if (g_key_slots[*key_id - PSA_LITE_KEY_ID_MIN].occupied == PSA_LITE_FALSE) {
			return PSA_ERROR_DOES_NOT_EXIST;
		}

		*slot = &g_key_slots[*key_id - PSA_LITE_KEY_ID_MIN].slot;
		return PSA_SUCCESS;
	}

	for (size_t key_cntr = 0; key_cntr < PSA_LITE_MAX_KEYS_SUPPORTED; key_cntr++) {
		if (g_key_slots[key_cntr].occupied == PSA_LITE_FALSE) {
			g_key_slots[key_cntr].occupied = PSA_LITE_TRUE;
			*key_id = key_cntr + PSA_LITE_KEY_ID_MIN;
			*slot = &g_key_slots[*key_id - PSA_LITE_KEY_ID_MIN].slot;
			return PSA_SUCCESS;
		}
	}
	return PSA_ERROR_INSUFFICIENT_MEMORY;
}
