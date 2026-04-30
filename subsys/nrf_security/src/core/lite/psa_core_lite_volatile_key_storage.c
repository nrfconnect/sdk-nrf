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

#define PSA_CORE_LITE_TRUE	0xA5FFA5FF
#define PSA_CORE_LITE_FALSE	0u

typedef struct {
	psa_core_lite_key_slot_t slot;
	uint32_t occupied;
} psa_core_lite_key_slot_entry_t;

static
psa_core_lite_key_slot_entry_t g_key_slots[CONFIG_PSA_CORE_LITE_MAX_VOLATILE_KEYS_COUNT] = {};

void psa_core_lite_free_key_slot(mbedtls_svc_key_id_t key_id)
{
	psa_core_lite_key_slot_entry_t *slot_entry;

	if (!psa_core_lite_key_id_is_volatile(key_id)) {
		return;
	}

	slot_entry = &g_key_slots[key_id - PSA_CORE_LITE_KEY_ID_MIN];
	safe_memzero(&slot_entry->slot, sizeof(psa_core_lite_key_slot_t));
	slot_entry->occupied = PSA_CORE_LITE_FALSE;
}

psa_status_t psa_core_lite_get_key_slot(mbedtls_svc_key_id_t *key_id,
					psa_core_lite_key_slot_t **slot)
{
	psa_core_lite_key_slot_entry_t *slot_entry;

	/* Note: it is assumed that key_id has already been verified for volatile key */
	if (key_id == NULL || slot == NULL) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (*key_id != PSA_CORE_LITE_KEY_ID_NULL) {
		slot_entry = &g_key_slots[*key_id - PSA_CORE_LITE_KEY_ID_MIN];

		if (slot_entry->occupied == PSA_CORE_LITE_FALSE) {
			return PSA_ERROR_DOES_NOT_EXIST;
		}

		*slot = &slot_entry->slot;
		return PSA_SUCCESS;
	}

	for (size_t cntr = 0; cntr < CONFIG_PSA_CORE_LITE_MAX_VOLATILE_KEYS_COUNT; cntr++) {
		slot_entry = &g_key_slots[cntr];

		if (slot_entry->occupied == PSA_CORE_LITE_FALSE) {
			slot_entry->occupied = PSA_CORE_LITE_TRUE;
			*key_id = cntr + PSA_CORE_LITE_KEY_ID_MIN;
			*slot = &slot_entry->slot;
			return PSA_SUCCESS;
		}
	}
	return PSA_ERROR_INSUFFICIENT_MEMORY;
}

void psa_core_lite_free_all_key_slots(void)
{
	safe_memzero(g_key_slots, sizeof(g_key_slots));
}
