/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/random/random.h>
#include <nrfx_kmu.h>
#include "wifi_kmu.h"

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

static bool key_is_mic(wifi_kmu_key_type_t type)
{
	return type == PEER_UCST_MIC || type == PEER_BCST_MIC || type == VIF_MIC;
}

uint32_t wifi_kmu_get_key_start_addr(wifi_kmu_key_type_t type, uint32_t db_id, uint32_t key_index)
{
	uint32_t db_base;
	uint32_t offset;
	bool mic = key_is_mic(type);

	if (type == VIF_ENC || type == VIF_MIC) {
		if (db_id >= NUM_VIF_ENTRIES) {
			return WIFI_KMU_KEY_ADDR_INVALID;
		}
		db_base = VIF_KEY_DB_OFFSET + db_id * VIF_DB_SIZE_PER_ENTRY;
		if (key_index >= NUM_KEYS_PER_ENTRY) {
			return WIFI_KMU_KEY_ADDR_INVALID;
		}
		if (mic) {
			offset = key_index * MIC_KEY_LEN;
		} else {
			offset = VIF_MIC_LEN_PER_ENTRY + key_index * ENC_KEY_LEN;
		}
	} else { /* PEER */
		if (db_id >= NUM_PEER_ENTRIES) {
			return WIFI_KMU_KEY_ADDR_INVALID;
		}
		db_base = db_id * PEER_DB_SIZE_PER_ENTRY;
		if (type == PEER_UCST_ENC || type == PEER_UCST_MIC) {
			offset = mic ? 0x0 : MIC_KEY_LEN;
		} else { /* PEER_BCST */
			if (key_index >= NUM_KEYS_PER_ENTRY) {
				return WIFI_KMU_KEY_ADDR_INVALID;
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

uint32_t wifi_kmu_get_key_size_in_bytes(wifi_kmu_key_type_t type)
{
	return key_is_mic(type) ? MIC_KEY_LEN : ENC_KEY_LEN;
}

uint32_t wifi_kmu_get_key_size_in_bits(wifi_kmu_key_type_t type)
{
	return wifi_kmu_get_key_size_in_bytes(type) * 8;
}

int wifi_kmu_key_reverse_byte_order(void *restrict dst, const void *restrict src,
				    uint32_t key_length_bytes)
{
	/* Fix byte order of keys from wpa_supplicant */

	uint8_t *d = dst;
	const uint8_t *s = src;
	const uint32_t group_size = 16;

	if (key_length_bytes % group_size != 0) {
		return -1;
	}

	for (uint32_t i = 0; i < key_length_bytes; i += group_size) {
		for (uint32_t j = 0; j < group_size; j++) {
			d[i + j] = s[i + group_size - 1 - j];
		}
	}

	return 0;
}

int wifi_kmu_key_reverse_byte_order_in_place(void *buf, uint32_t key_length_bytes)
{
	/* Fix byte order of keys from wpa_supplicant - in place version */

	uint8_t *b = buf;
	const uint32_t group_size = 16;

	if (key_length_bytes % group_size != 0) {
		return -1;
	}

	for (uint32_t i = 0; i < key_length_bytes; i += group_size) {
		for (uint32_t j = 0; j < group_size / 2; j++) {
			uint8_t tmp = b[i + j];
			b[i + j] = b[i + group_size - 1 - j];
			b[i + group_size - 1 - j] = tmp;
		}
	}

	return 0;
}

uint32_t wifi_kmu_get_next_slot(uint32_t key_length_bytes)
{
	static bool init = false;
	static uint32_t next_slot = 0;

	if (!init) {
		/* Randomise starting slot.
		   To ensure uniform distribution, find largest multiple of
		   CONFIG_NRF_WIFI_KMU_NUM_SLOTS smaller than or equal to 255 */
		uint8_t rand_max =
			(UINT8_MAX / CONFIG_NRF_WIFI_KMU_NUM_SLOTS) * CONFIG_NRF_WIFI_KMU_NUM_SLOTS;
		uint8_t rand8;

		do {
			rand8 = sys_rand8_get();
		} while (rand8 >= rand_max);

		next_slot = rand8 % CONFIG_NRF_WIFI_KMU_NUM_SLOTS;
		init = true;
	}

	const uint32_t bytes_per_slot = KMU_KEYSLOTBITS / 8;

	if (key_length_bytes % bytes_per_slot != 0) {
		return WIFI_KMU_KEY_LENGTH_INVALID;
	}

	uint32_t slot;

	uint32_t req_slots = key_length_bytes / bytes_per_slot;
	if (next_slot + req_slots > CONFIG_NRF_WIFI_KMU_NUM_SLOTS) {
		next_slot = 0;
	}

	slot = next_slot;
	next_slot += req_slots;
	return slot + CONFIG_NRF_WIFI_KMU_SLOT_MIN;
}
