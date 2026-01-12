/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_keys, CONFIG_WIFI_KEYS_LOG_LEVEL);

#include <stdint.h>
#include <cracen/lib_kmu.h>

#include <zephyr/sys/__assert.h>

#include "wifi_keys.h"

static const uint32_t WIFI_KEYS_KEY_INDEX_INVALID = ~0;

static bool wifi_keys_key_type_is_mic(wifi_keys_key_type_t type)
{
	return type == PEER_UCST_MIC || type == PEER_BCST_MIC || type == VIF_MIC;
}

static uint32_t wifi_keys_kmu_slot_id(wifi_keys_key_type_t type, uint32_t db_id, uint32_t key_index)
{
	/* TODO Coordinate with other users to reserve slots for Wi-Fi. */

	(void)type;
	(void)db_id;
	(void)key_index;

	return 97;
}

static int wifi_keys_set_key_id(psa_key_attributes_t *attr, uint32_t db_id, uint32_t key_index)
{
	if (db_id >= 8) {
		LOG_ERR("Invalid db_id: %d", db_id);
		return 1;
	}
	if (key_index >= 4) {
		LOG_ERR("Invalid key_index: %d", key_index);
		return 1;
	}

	/* Arbitrary key ID scheme - non-builtin key. */
	psa_key_id_t id = 0x3F000000 | ('W' << 16) | ('C' << 8) | (db_id << 2) | (key_index);

	psa_set_key_id(attr, id);
	return 0;
}

static uint32_t wifi_keys_get_key_start_addr(wifi_keys_key_type_t type, uint32_t db_id,
					     uint32_t key_index)
{
	const uint32_t ram_base = 0x28400000;
	uint32_t db_base;
	uint32_t offset;
	bool mic = wifi_keys_key_type_is_mic(type);

	if (type == VIF_ENC || type == VIF_MIC) {
		if (db_id >= 4) {
			return WIFI_KEYS_KEY_INDEX_INVALID;
		}
		db_base = 0x1000 + db_id * 0xC0;
		if (key_index >= 4) {
			return WIFI_KEYS_KEY_INDEX_INVALID;
		}
		if (mic) {
			offset = key_index * 0x10;
		} else {
			offset = 0x40 + key_index * 0x20;
		}
	} else { /* PEER */
		if (db_id >= 8) {
			return WIFI_KEYS_KEY_INDEX_INVALID;
		}
		db_base = db_id * 0xF0;
		if (type == PEER_UCST_ENC || type == PEER_UCST_MIC) {
			offset = mic ? 0x0 : 0x10;
		} else { /* PEER_BCST */
			if (key_index >= 4) {
				return WIFI_KEYS_KEY_INDEX_INVALID;
			}
			if (mic) {
				offset = 0x30 + key_index * 0x10;
			} else {
				offset = 0x70 + key_index * 0x20;
			}
		}
	}

	return ram_base + db_base + offset;
}

uint32_t wifi_keys_get_key_size_in_bytes(wifi_keys_key_type_t type)
{
	return wifi_keys_key_type_is_mic(type) ? 16 : 32;
}

uint32_t wifi_keys_get_key_size_in_bits(wifi_keys_key_type_t type)
{
	return wifi_keys_get_key_size_in_bytes(type) * 8;
}

static int wifi_keys_kmu_provision_and_push(const uint32_t *key, wifi_keys_key_type_t type,
					    uint32_t db_id, uint32_t key_index, uint32_t key_slot)
{
	struct kmu_src src;

	memcpy(&src.value, key, 16);
	src.rpolicy = LIB_KMU_REV_POLICY_ROTATING;
	src.dest = wifi_keys_get_key_start_addr(type, db_id, key_index);
	if (src.dest == WIFI_KEYS_KEY_INDEX_INVALID) {
		LOG_ERR("Invalid destination address (db_id: %d, key_index: %d)", db_id, key_index);
		return 1;
	}
	src.metadata = 0;
	int ret = lib_kmu_provision_slot(key_slot, &src);

	if (ret) {
		LOG_ERR("Failed to provision key (db_id: %d, key_index: %d): %d", db_id, key_index,
			ret);
		return ret;
	}
	ret = lib_kmu_push_slot(key_slot);
	if (ret) {
		LOG_ERR("Failed to push key (db_id: %d, key_index: %d): %d", db_id, key_index, ret);
		return ret;
	}

	if (wifi_keys_get_key_size_in_bytes(type) == 32) {
		memcpy(&src.value, key + 4, 16);
		src.dest += 16;

		ret = lib_kmu_provision_slot(key_slot + 1, &src);
		if (ret) {
			return ret;
		}
		ret = lib_kmu_push_slot(key_slot + 1);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

psa_key_attributes_t wifi_keys_key_attributes_init(wifi_keys_key_type_t type, uint32_t db_id,
						   uint32_t key_index)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	wifi_keys_set_key_id(&attr, db_id, key_index);

	psa_key_persistence_t persistence = PSA_KEY_PERSISTENCE_DEFAULT;

	psa_key_lifetime_t lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
		persistence, PSA_KEY_LOCATION_WIFI_KEYS);

	psa_set_key_lifetime(&attr, lifetime);
	psa_set_key_type(&attr, (psa_key_type_t)type);

	return attr;

	/* Note: Usage flags and algorithm are deliberately not set (kept at 0),
	 * because software has no permission to use this key for any purpose
	 * (only Wi-Fi Crypto hardware can use it), and because Wi-Fi Crypto
	 * key locations can be reused for different algorithms).
	 */
}

psa_status_t wifi_keys_import_key(const psa_key_attributes_t *attr, const uint8_t *data,
				  size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
				  size_t *key_buffer_length, size_t *key_bits)
{
	__ASSERT_NO_MSG(key_buffer);
	__ASSERT_NO_MSG(key_buffer_length);
	__ASSERT_NO_MSG(key_bits);

	if (key_buffer_size == 0) {
		LOG_ERR("Invalid key buffer size: %d", key_buffer_size);
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_key_lifetime_t lifetime = psa_get_key_lifetime(attr);
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(lifetime);
	psa_key_persistence_t persistence = PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime);
	mbedtls_svc_key_id_t key = psa_get_key_id(attr);

	LOG_INF("Importing key to PSA, location: %d, persistence: %d, lifetime: %d key: 0x%08X",
		location, persistence, lifetime, key);
	if (location == PSA_KEY_LOCATION_WIFI_KEYS) {
		wifi_keys_key_type_t type = (wifi_keys_key_type_t)psa_get_key_type(attr);
		psa_key_id_t id = psa_get_key_id(attr);

		uint32_t db_id = (id >> 2) & 0x7;
		uint32_t key_index = id & 0x3;

		LOG_INF("Key ID: %d, DB ID: %d, Key Index: %d", id, db_id, key_index);

		if (data_length != wifi_keys_get_key_size_in_bytes(type)) {
			LOG_ERR("Invalid key data length: %d, expected: %d", data_length,
				wifi_keys_get_key_size_in_bytes(type));
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* Output parameters not used, set to arbitrary values. */
		*key_buffer_length = 32; /* max key size in bytes */
		*key_bits = wifi_keys_get_key_size_in_bits(type);
		memset(key_buffer, 0, *key_buffer_length);

		if (persistence == PSA_KEY_PERSISTENCE_DEFAULT) {
			uint32_t key_slot = wifi_keys_kmu_slot_id(type, db_id, key_index);
			int ret = wifi_keys_kmu_provision_and_push((const uint32_t *)data, type,
								   db_id, key_index, key_slot);
			if (ret) {
				LOG_ERR("Failed to provision and push key: %d", ret);
			}
			return ret;
		}
		LOG_ERR("Invalid persistence: %d", persistence);
		return PSA_ERROR_INVALID_ARGUMENT;
	}
	return PSA_ERROR_NOT_SUPPORTED;
}
