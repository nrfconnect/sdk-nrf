/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include "wifi_keys.h"

#include "cracen_psa.h"

static const uint32_t WIFI_KEYS_KMU_SLOT_ID = 181;
static const uint32_t KMU_PUSH_KEY_ID =
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, WIFI_KEYS_KMU_SLOT_ID);

/* Wi-Fi Keys sizes */
static const uint32_t MIC_KEY_LEN = 0x10;
static const uint32_t ENC_KEY_LEN = 0x20;

/* Wi-Fi Keys RAM structure */
static const uint32_t RAM_BASE = 0x28400000;
static const uint32_t NUM_KEYS_PER_ENTRY = 4;

/* VIF keys: (4 MIC keys, then 4 encryption keys) * 4 entries */
static const uint32_t VIF_KEY_DB_OFFSET = 0x1000;
static const uint32_t VIF_MIC_LEN_PER_ENTRY = MIC_KEY_LEN * NUM_KEYS_PER_ENTRY;
static const uint32_t VIF_KEY_LEN_PER_ENTRY = ENC_KEY_LEN * NUM_KEYS_PER_ENTRY;
static const uint32_t VIF_DB_SIZE_PER_ENTRY = VIF_MIC_LEN_PER_ENTRY + VIF_KEY_LEN_PER_ENTRY;
static const uint32_t NUM_VIF_ENTRIES = 4;

/* Peer keys:
 * (UCST MIC key, UCST encryption key, 4 BCST MIC keys, 4 BCST encryption keys) * 8 entries */
static const uint32_t PEER_UCST_MIC_LEN_PER_ENTRY = MIC_KEY_LEN;
static const uint32_t PEER_UCST_KEY_LEN_PER_ENTRY = ENC_KEY_LEN;
static const uint32_t PEER_BCST_MIC_LEN_PER_ENTRY = MIC_KEY_LEN * NUM_KEYS_PER_ENTRY;
static const uint32_t PEER_BCST_KEY_LEN_PER_ENTRY = ENC_KEY_LEN * NUM_KEYS_PER_ENTRY;
static const uint32_t PEER_UCST_SIZE_PER_ENTRY =
	PEER_UCST_MIC_LEN_PER_ENTRY + PEER_UCST_KEY_LEN_PER_ENTRY;
static const uint32_t PEER_BCST_SIZE_PER_ENTRY =
	PEER_BCST_MIC_LEN_PER_ENTRY + PEER_BCST_KEY_LEN_PER_ENTRY;
static const uint32_t PEER_DB_SIZE_PER_ENTRY = PEER_UCST_SIZE_PER_ENTRY + PEER_BCST_SIZE_PER_ENTRY;
static const uint32_t NUM_PEER_ENTRIES = 8;

static bool wifi_keys_is_mic(wifi_keys_type_t type)
{
	return type == PEER_UCST_MIC || type == PEER_BCST_MIC || type == VIF_MIC;
}

uint32_t wifi_keys_get_key_start_addr(wifi_keys_type_t type, uint32_t db_id, uint32_t key_index)
{
	uint32_t db_base;
	uint32_t offset;
	bool mic = wifi_keys_is_mic(type);

	if (type == VIF_ENC || type == VIF_MIC) {
		if (db_id >= NUM_VIF_ENTRIES) {
			return WIFI_KEYS_ADDR_INVALID;
		}
		db_base = VIF_KEY_DB_OFFSET + db_id * VIF_DB_SIZE_PER_ENTRY;
		if (key_index >= NUM_KEYS_PER_ENTRY) {
			return WIFI_KEYS_ADDR_INVALID;
		}
		if (mic) {
			offset = key_index * MIC_KEY_LEN;
		} else {
			offset = VIF_MIC_LEN_PER_ENTRY + key_index * ENC_KEY_LEN;
		}
	} else { /* PEER */
		if (db_id >= NUM_PEER_ENTRIES) {
			return WIFI_KEYS_ADDR_INVALID;
		}
		db_base = db_id * PEER_DB_SIZE_PER_ENTRY;
		if (type == PEER_UCST_ENC || type == PEER_UCST_MIC) {
			offset = mic ? 0x0 : MIC_KEY_LEN;
		} else { /* PEER_BCST */
			if (key_index >= NUM_KEYS_PER_ENTRY) {
				return WIFI_KEYS_ADDR_INVALID;
			}
			if (mic) {
				offset = PEER_UCST_SIZE_PER_ENTRY + key_index * MIC_KEY_LEN;
			} else {
				offset = PEER_UCST_SIZE_PER_ENTRY + PEER_BCST_MIC_LEN_PER_ENTRY +
					 key_index * ENC_KEY_LEN;
			}
		}
	}

	return RAM_BASE + db_base + offset;
}

uint32_t wifi_keys_get_key_size_in_bytes(wifi_keys_type_t type)
{
	return wifi_keys_is_mic(type) ? MIC_KEY_LEN : ENC_KEY_LEN;
}

uint32_t wifi_keys_get_key_size_in_bits(wifi_keys_type_t type)
{
	return PSA_BYTES_TO_BITS(wifi_keys_get_key_size_in_bytes(type));
}

psa_key_attributes_t wifi_keys_key_attributes_init(uint32_t key_length_bytes)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	psa_key_id_t id = KMU_PUSH_KEY_ID;

	psa_set_key_id(&attr, id);

	psa_key_persistence_t persistence = PSA_KEY_PERSISTENCE_DEFAULT;

	psa_key_lifetime_t lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
		persistence, PSA_KEY_LOCATION_KMU_PUSH);

	psa_set_key_lifetime(&attr, lifetime);
	psa_set_key_bits(&attr, PSA_BYTES_TO_BITS(key_length_bytes));
	psa_set_key_type(&attr, PSA_KEY_TYPE_VENDOR_FLAG);

	return attr;

	/* Note: Usage flags and algorithm are deliberately not set (kept at 0),
	 * because software has no permission to use this key for any purpose
	 * (only Wi-Fi Crypto hardware can use it), and because Wi-Fi Crypto
	 * key locations can be reused for different algorithms).
	 */
}
