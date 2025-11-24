#include <stdint.h>
#include <cracen/lib_kmu.h>
#include "nrf.h"

#include <zephyr/sys/__assert.h>

#include "wifi_keys.h"

static const uint32_t WIFI_KEYS_KEY_INDEX_INVALID = ~0;

static bool wifi_keys_key_type_is_mic(wifi_keys_key_type_t type)
{
	return (bool)(type & 1);
}

static uint32_t wifi_keys_kmu_slot_id(wifi_keys_key_type_t type, uint32_t db_id, uint32_t key_index)
{
	/* TODO Can we use all these key slots?
	 * Do we need to reserve some for other uses?
	 * Is reusing the last slot a problem? */

	const uint32_t cracen_max_kmu_slot = 4; /* See cracen_psa_kmu.h and cracen_pas_key_ids.h */
	uint32_t slot = cracen_max_kmu_slot + 1;

	uint32_t group =
		(type & ~PSA_KEY_TYPE_VENDOR_FLAG) >> 1; /* PEER_UCST = 0, PEER_BCST = 1, VIF = 2 */

	/* Each MIC key uses one slot, and each ENC key uses two slots. */
	if (wifi_keys_key_type_is_mic(type)) {
		slot += (group << 5) | (db_id << 2) | (key_index);
	} else {
		slot += (3 << 5);
		slot += ((group << 5) | (db_id << 2) | (key_index)) * 2;
	}

	/* Reuse last slot if we don't have enough */
	if (slot > KMU_KEYSLOT_ID_Max) {
		slot = KMU_KEYSLOT_ID_Max;
	}

	return slot;
}

static int wifi_keys_set_key_id(psa_key_attributes_t *attr, uint32_t db_id, uint32_t key_index)
{
	if (db_id >= 8) {
		return 1;
	}
	if (key_index >= 4) {
		return 1;
	}

	/* Arbitrary key ID scheme. Can be changed if necessary.
	 * Note that if we need to change to a "builtin key" (not sure what that means)
	 * then we need to implement a wifi_keys_destroy_key function,
	 * and extend psa_crypto_driver_wrappers.c psa_driver_wrapper_destroy_builtin_key
	 * to call it. */
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
	} else { /* PEER_* */
		if (db_id >= 8) {
			return WIFI_KEYS_KEY_INDEX_INVALID;
		}
		db_base = db_id * 0xF0;
		if (type == PEER_UCST_ENC || type == PEER_UCST_MIC) {
			offset = mic ? 0x0 : 0x10;
		} else { /* PEER_BCST_* */
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
		return 1;
	}
	src.metadata = 0;
	int ret = lib_kmu_provision_slot(key_slot, &src);
	if (ret) {
		return ret;
	}
	ret = lib_kmu_push_slot(key_slot);
	if (ret) {
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
	 * key locations can be reused for different algorithms). */
}

psa_status_t wifi_keys_import_key(const psa_key_attributes_t *attr, const uint8_t *data,
				  size_t data_length, uint8_t *key_buffer, size_t key_buffer_size,
				  size_t *key_buffer_length, size_t *key_bits)
{
	__ASSERT_NO_MSG(key_buffer);
	__ASSERT_NO_MSG(key_buffer_length);
	__ASSERT_NO_MSG(key_bits);

	if (key_buffer_size == 0) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	psa_key_lifetime_t lifetime = psa_get_key_lifetime(attr);
	psa_key_location_t location = PSA_KEY_LIFETIME_GET_LOCATION(lifetime);
	psa_key_persistence_t persistence = PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime);

	if (location == PSA_KEY_LOCATION_WIFI_KEYS) {
		wifi_keys_key_type_t type = (wifi_keys_key_type_t)psa_get_key_type(attr);
		psa_key_id_t id = psa_get_key_id(attr);

		uint32_t db_id = (id >> 2) & 0x7;
		uint32_t key_index = id & 0x3;

		if (data_length != wifi_keys_get_key_size_in_bytes(type)) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		/* I don't understand the purpose of these output paramenters.
		 * Setting them to arbitrary values for now. */
		*key_buffer_length = 32;
		*key_bits = wifi_keys_get_key_size_in_bits(type);
		memset(key_buffer, 0, *key_buffer_length);

		if (persistence == PSA_KEY_PERSISTENCE_DEFAULT) {
			uint32_t key_slot = wifi_keys_kmu_slot_id(type, db_id, key_index);
			return wifi_keys_kmu_provision_and_push((const uint32_t *)data, type, db_id,
								key_index, key_slot);
		} else {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
	}
	return PSA_ERROR_NOT_SUPPORTED;
}
