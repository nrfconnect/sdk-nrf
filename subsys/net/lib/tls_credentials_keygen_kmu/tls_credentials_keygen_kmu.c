/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/net/tls_credentials_keygen_store.h>
#include <zephyr/sys/util.h>
#include <psa/crypto.h>
#include <cracen_psa_kmu.h>

#if defined(CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SCHEME_ENCRYPTED)
#define SCHEME     CRACEN_KMU_KEY_USAGE_SCHEME_ENCRYPTED
/* Key slots, and two slots for the nonce and the tag. */
#define SLOTS_P256 4
#define SLOTS_P384 5
#else
#define SCHEME     CRACEN_KMU_KEY_USAGE_SCHEME_RAW
#define SLOTS_P256 2
#define SLOTS_P384 3
#endif

/* Each key gets a block of slots that fits the largest enabled key type, so that a slot of one
 * key is never taken for the first slot of another key.
 */
#define BLOCK_SLOTS                                                                                \
	(IS_ENABLED(CONFIG_TLS_CREDENTIALS_KEYGEN_SECP384R1) ? SLOTS_P384 : SLOTS_P256)
#define SLOT_FIRST CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_SLOT_FIRST
#define KEY_COUNT  CONFIG_TLS_CREDENTIALS_KEYGEN_KMU_KEY_COUNT
#define SLOT_END   (SLOT_FIRST + KEY_COUNT * BLOCK_SLOTS)

/* The non-secure image cannot use the KMU slots from 180. */
BUILD_ASSERT(SLOT_END <= 180, "The TLS key slots must end before KMU slot 180");

#if defined(CONFIG_NRF_WIFI_KMU)
BUILD_ASSERT(SLOT_END <= CONFIG_NRF_WIFI_KMU_SLOT_MIN ||
		     SLOT_FIRST >= CONFIG_NRF_WIFI_KMU_SLOT_MIN + CONFIG_NRF_WIFI_KMU_NUM_SLOTS,
	     "The TLS key slots overlap the Wi-Fi KMU slots");
#endif

static psa_key_id_t key_id_get(size_t index)
{
	return PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(SCHEME, SLOT_FIRST + index * BLOCK_SLOTS);
}

int tls_credential_keygen_store_attr_set(enum tls_credential_keygen_type type,
					 psa_key_attributes_t *attr)
{
	/* The default persistence lets a deleted key free its slots. */
	psa_key_lifetime_t lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
		PSA_KEY_PERSISTENCE_DEFAULT, PSA_KEY_LOCATION_CRACEN_KMU);

	ARG_UNUSED(type);

	psa_set_key_lifetime(attr, lifetime);

	return 0;
}

bool tls_credential_keygen_store_id_is_own(psa_key_id_t key_id)
{
	unsigned int slot = CRACEN_PSA_GET_KMU_SLOT(key_id);

	if (slot < SLOT_FIRST || slot >= SLOT_END || (slot - SLOT_FIRST) % BLOCK_SLOTS != 0) {
		return false;
	}

	return key_id == key_id_get((slot - SLOT_FIRST) / BLOCK_SLOTS);
}

int tls_credential_keygen_store_id_alloc(sec_tag_t tag, enum tls_credential_keygen_type type,
					 const psa_key_id_t *used, size_t used_count,
					 psa_key_id_t *key_id)
{
	ARG_UNUSED(tag);
	ARG_UNUSED(type);

	return tls_credential_keygen_store_id_pick(KEY_COUNT, key_id_get, used, used_count, key_id);
}
