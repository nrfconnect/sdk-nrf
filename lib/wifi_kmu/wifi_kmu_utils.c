/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/random/random.h>
#include <nrfx_kmu.h>
#include <wifi_kmu/wifi_kmu.h>

/* Wi-Fi Keys sizes */
static const uint32_t mic_key_len = 0x10;
static const uint32_t enc_key_len = 0x20;

/* Wi-Fi Keys RAM structure */
static const uint32_t ram_base = 0x28400000;
static const uint32_t num_keys_per_entry = 4;

/* VIF keys: (4 MIC keys, then 4 encryption keys) * 4 entries */
static const uint32_t vif_key_db_offset = 0x1000;
static const uint32_t vif_mic_len_per_entry = mic_key_len * num_keys_per_entry;
static const uint32_t vif_key_len_per_entry = enc_key_len * num_keys_per_entry;
static const uint32_t vif_db_size_per_entry = vif_mic_len_per_entry + vif_key_len_per_entry;
static const uint32_t num_vif_entries = 4;

/* Peer keys:
 * (UCST MIC key, UCST encryption key, 4 BCST MIC keys, 4 BCST encryption keys) * 8 entries
 */
static const uint32_t peer_ucst_mic_len_per_entry = mic_key_len;
static const uint32_t peer_ucst_key_len_per_entry = enc_key_len;
static const uint32_t peer_bcst_mic_len_per_entry = mic_key_len * num_keys_per_entry;
static const uint32_t peer_bcst_key_len_per_entry = enc_key_len * num_keys_per_entry;
static const uint32_t peer_ucst_size_per_entry =
	peer_ucst_mic_len_per_entry + peer_ucst_key_len_per_entry;
static const uint32_t peer_bcst_size_per_entry =
	peer_bcst_mic_len_per_entry + peer_bcst_key_len_per_entry;
static const uint32_t peer_db_size_per_entry = peer_ucst_size_per_entry + peer_bcst_size_per_entry;
static const uint32_t num_peer_entries = 8;

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
		if (db_id >= num_vif_entries) {
			return wifi_kmu_key_addr_invalid;
		}
		db_base = vif_key_db_offset + db_id * vif_db_size_per_entry;
		if (key_index >= num_keys_per_entry) {
			return wifi_kmu_key_addr_invalid;
		}
		if (mic) {
			offset = key_index * mic_key_len;
		} else {
			offset = vif_mic_len_per_entry + key_index * enc_key_len;
		}
	} else { /* PEER */
		if (db_id >= num_peer_entries) {
			return wifi_kmu_key_addr_invalid;
		}
		db_base = db_id * peer_db_size_per_entry;
		if (type == PEER_UCST_ENC || type == PEER_UCST_MIC) {
			offset = mic ? 0x0 : mic_key_len;
		} else { /* PEER_BCST */
			if (key_index >= num_keys_per_entry) {
				return wifi_kmu_key_addr_invalid;
			}
			if (mic) {
				offset = peer_ucst_size_per_entry + key_index * mic_key_len;
			} else {
				offset = peer_ucst_size_per_entry + peer_bcst_mic_len_per_entry +
					 key_index * enc_key_len;
			}
		}
	}

	return ram_base + db_base + offset;
}

uint32_t wifi_kmu_get_key_size_in_bytes(wifi_kmu_key_type_t type)
{
	return key_is_mic(type) ? mic_key_len : enc_key_len;
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
	static bool init;
	static uint32_t next_slot;
	const uint32_t bytes_per_slot = KMU_KEYSLOTBITS / 8;
	uint32_t slot;
	uint32_t req_slots;

	if (!init) {
		/* Randomise starting slot.
		 * To ensure uniform distribution, find largest multiple of
		 * CONFIG_NRF_WIFI_KMU_NUM_SLOTS smaller than or equal to 255
		 */
		uint8_t rand_max =
			(UINT8_MAX / CONFIG_NRF_WIFI_KMU_NUM_SLOTS) * CONFIG_NRF_WIFI_KMU_NUM_SLOTS;
		uint8_t rand8;

		do {
			rand8 = sys_rand8_get();
		} while (rand8 >= rand_max);

		next_slot = rand8 % CONFIG_NRF_WIFI_KMU_NUM_SLOTS;
		init = true;
	}

	if (key_length_bytes % bytes_per_slot != 0) {
		return wifi_kmu_key_length_invalid;
	}

	req_slots = key_length_bytes / bytes_per_slot;

	if (next_slot + req_slots > CONFIG_NRF_WIFI_KMU_NUM_SLOTS) {
		next_slot = 0;
	}

	slot = next_slot;
	next_slot += req_slots;
	return slot + CONFIG_NRF_WIFI_KMU_SLOT_MIN;
}
