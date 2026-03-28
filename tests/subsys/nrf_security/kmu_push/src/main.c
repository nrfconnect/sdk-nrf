/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <psa/crypto.h>
#include <cracen_psa.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>
#include <cracen/cracen_kmu.h>
#include <cracen/mem_helpers.h>
#include <kmu_push.h>
#include <cracen/lib_kmu.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kmu_push_test, LOG_LEVEL_INF);

/** @brief Max number of slots for a KMU push key */
#define MAX_NUM_KMU_SLOTS_FOR_KEY (4)

/* Max key size for this test */
#define MAX_KEY_SIZE (CRACEN_KMU_SLOT_KEY_SIZE * MAX_NUM_KMU_SLOTS_FOR_KEY)

/** @brief Key size for revokable key */
#define REVOKABLE_KEY_SIZE (48)

/** @brief Key size for read-only key */
#define READ_ONLY_KEY_SIZE (32)

/** @brief Lowest KMU slot for the tests */
#define TEST_BASE_KMU_SLOT_ID (0)

/** @brief Key ID for a KMU push key */
#define KMU_PUSH_KEY_ID \
	PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, \
		TEST_BASE_KMU_SLOT_ID)

/** @brief Buffer used for key-material to be pushed */
static uint8_t dest_buffer[CRACEN_KMU_SLOT_KEY_SIZE * MAX_NUM_KMU_SLOTS_FOR_KEY];

/** @brief Key attributes */
static psa_key_attributes_t key_attributes = {0};

/** @brief test pattern for a cryptographic key to push */
const uint8_t test_key[16] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

BUILD_ASSERT(sizeof(test_key) == CRACEN_KMU_SLOT_KEY_SIZE, "Update the test_key to new KMU size");

/** @brief Clear the RAM buffer */
static void clear_dest_buffer(void) {
	safe_memzero(dest_buffer, sizeof(dest_buffer));
}

/** @brief Fill the key with the test pattern */
static void fill_key(uint8_t *key_buffer, size_t key_size)
{
	size_t num_slots = DIV_ROUND_UP(key_size, CRACEN_KMU_SLOT_KEY_SIZE);
	size_t block_size = 0;
	size_t size_left = key_size;

	for(size_t i = 0; i < num_slots; i++) {
		block_size = size_left > CRACEN_KMU_SLOT_KEY_SIZE ? CRACEN_KMU_SLOT_KEY_SIZE : size_left;
		memcpy(key_buffer, test_key, block_size);
		key_buffer += block_size;
		size_left -= block_size;
	}
}

static bool check_all_filled(uint32_t slot_id, size_t key_size)
{
	bool is_filled = true;
	size_t num_slots = DIV_ROUND_UP(key_size, CRACEN_KMU_SLOT_KEY_SIZE);

	for (size_t i = 0; i < num_slots; i++) {
		if (lib_kmu_is_slot_empty(slot_id + i)) {
			is_filled = false;
		}
	}

	return is_filled;
}

static bool check_none_filled(uint32_t slot_id, size_t key_size)
{
	bool is_empty = true;
	size_t num_slots = DIV_ROUND_UP(key_size, CRACEN_KMU_SLOT_KEY_SIZE);

	for (size_t i = 0; i < num_slots; i++) {
		if (!lib_kmu_is_slot_empty(slot_id + i)) {
			is_empty = false;
		}
	}

	return is_empty;
}

static bool check_key_value(uint8_t *key_buffer, size_t key_size)
{
	if (constant_memcmp(dest_buffer, key_buffer, key_size) == 0) {
		return true;
	} else {
		return false;
	}
}

static psa_status_t test_import_key(mbedtls_svc_key_id_t key_id, psa_key_persistence_t persistence,
				    uint32_t usage_mask, uint8_t * key_buffer, size_t key_size)
{
	psa_key_lifetime_t lifetime =
		PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			persistence, PSA_KEY_LOCATION_KMU_PUSH);

	kmu_push_key_buffer_t kmu_push_buffer = {
		.usage_mask = usage_mask,
		.dest_address = (uint32_t) &dest_buffer,
		.buffer_size = key_size,
	};

	clear_dest_buffer();
	fill_key(key_buffer, key_size);
	kmu_push_buffer.key_buffer = key_buffer;

	safe_memzero(&key_attributes, sizeof(psa_key_attributes_t));
	psa_set_key_lifetime(&key_attributes, lifetime);
	psa_set_key_bits(&key_attributes, PSA_BYTES_TO_BITS(key_size));
	psa_set_key_id(&key_attributes, key_id);

	return psa_import_key(&key_attributes, (uint8_t *) &kmu_push_buffer, sizeof(kmu_push_buffer), &key_id);
}

static psa_status_t test_destroy_key(mbedtls_svc_key_id_t key_id)
{
	return psa_destroy_key(key_id);
}

static void clear_kmu_slots()
{
	int err;
	/* Clear out all slots prior to testing*/
	for(size_t i = 0; i <= MAX_NUM_KMU_SLOTS_FOR_KEY; i++) {
		if  (lib_kmu_is_slot_empty(i)) {
			continue;
		}

		err = lib_kmu_revoke_slot(i);
		zassert_equal(err, LIB_KMU_SUCCESS, "Failed to clear KMU slot: %i", i);
	}
}

static void test_rotatable_import(void)
{
	psa_status_t err;
	uint8_t test_key[MAX_KEY_SIZE] = {0};
	mbedtls_svc_key_id_t key_id = KMU_PUSH_KEY_ID;
	psa_key_persistence_t persistence = PSA_KEY_PERSISTENCE_DEFAULT;
	uint32_t usage_flags = KMU_PUSH_USAGE_MASK_NONE;

	LOG_DBG("Test rotatable: Import");

	for (size_t i = 8; i <= MAX_KEY_SIZE; i+=8) {
		LOG_DBG("Test rotatable: Import: key_size: %d", i);
		err = test_import_key(key_id, persistence, usage_flags, test_key, i);

		zassert_equal(err, PSA_SUCCESS,
			      "Rotatable key (import): Import failed for size: %d, err: %d", i, err);
		/* Only possible to check that KMU slot is filled as there is no push */
		zassert_true(check_all_filled(key_id, i),
			     "Rotatable key (import): Key not present for size: %d", i);
		/* Rotatable key can be erased */
		err = test_destroy_key(key_id);
		zassert_equal(err, PSA_SUCCESS,
			      "Rotatable key (import): Destroy failed for size: %d, err: %d", i, err);
	}
}

static void test_rotatable_import_push(void) {
	psa_status_t err;
	uint8_t test_key[MAX_KEY_SIZE] = {0};
	mbedtls_svc_key_id_t key_id = KMU_PUSH_KEY_ID;
	psa_key_persistence_t persistence = PSA_KEY_PERSISTENCE_DEFAULT;
	uint32_t usage_flags = KMU_PUSH_USAGE_MASK_PUSH_IMMEDIATELY;

	LOG_DBG("Test rotatable: Import, push");

	for (size_t i = 8; i <= MAX_KEY_SIZE; i+=8) {
		LOG_DBG("Test rotatable: Import, push: key_size: %d", i);
		err = test_import_key(key_id, persistence, usage_flags, test_key, i);
		zassert_equal(err, PSA_SUCCESS,
			      "Rotatable key (import, push): Import failed for size: %d, err: %d", i, err);
		/* Check that key RAM is correct */
		zassert_true(check_key_value(test_key, i),
			     "Rotatable key (import, push): Key value mismatch for size: %d, err %d", i, err);
		/* Rotatable key can be erased */
		err = test_destroy_key(key_id);
		zassert_equal(err, PSA_SUCCESS,
			      "Rotatable key (import, push): Destroy failed for size: %d, err: %d", i, err);
	}
}

static void test_rotatable_import_push_destroy(void) {
	psa_status_t err;
	uint8_t test_key[MAX_KEY_SIZE] = {0};
	mbedtls_svc_key_id_t key_id = KMU_PUSH_KEY_ID;
	psa_key_persistence_t persistence = PSA_KEY_PERSISTENCE_DEFAULT;
	uint32_t usage_flags = KMU_PUSH_USAGE_MASK_PUSH_IMMEDIATELY | KMU_PUSH_USAGE_MASK_DESTROY_AFTER;

	LOG_DBG("Test rotatable: Import, push, destroy");
	for (size_t i = 8; i <= MAX_KEY_SIZE; i+=8) {
		LOG_DBG("Test rotatable: Import, push, destroy: key_size: %d", i);
		err = test_import_key(key_id, persistence, usage_flags, test_key, i);
		zassert_equal(err, PSA_SUCCESS,
			      "Rotatable key (import, push, destroy): Import failed for size: %d, err: %d", i, err);
		/* Check that key RAM is correct */
		zassert_true(check_key_value(test_key, i),
			     "Rotatable key (import, push, destroy): Key value mismatch for size: %d, err %d", i, err);
		/* Rotatable key already erased. Check that KMU is empty */
		zassert_true(check_none_filled(key_id, i),
			     "Rotatable key (import, push, destroy): Key still present for size: %d, err: %d", i, err);
	}
}

static void test_revokable_import_not_supported(void) {
	psa_status_t err;
	uint8_t test_key[MAX_KEY_SIZE] = {0};
	mbedtls_svc_key_id_t key_id = KMU_PUSH_KEY_ID;
	psa_key_persistence_t persistence = CRACEN_KEY_PERSISTENCE_REVOKABLE;
	uint32_t usage_flags = KMU_PUSH_USAGE_MASK_PUSH_IMMEDIATELY | KMU_PUSH_USAGE_MASK_DESTROY_AFTER;
	const size_t key_size = REVOKABLE_KEY_SIZE;

	LOG_DBG("Test revokable: Import, push, destroy");
	err = test_import_key(key_id, persistence, usage_flags, test_key, key_size);
	zassert_equal(err, PSA_ERROR_NOT_SUPPORTED,
		      "Revokable key: Import should fail with PSA_ERROR_NOT_SUPPORTED, err: %d", err);
}

static void test_read_only_import_not_supported(void) {
	psa_status_t err;
	uint8_t test_key[MAX_KEY_SIZE] = {0};
	mbedtls_svc_key_id_t key_id = KMU_PUSH_KEY_ID;
	psa_key_persistence_t persistence = CRACEN_KEY_PERSISTENCE_READ_ONLY;
	uint32_t usage_flags = KMU_PUSH_USAGE_MASK_PUSH_IMMEDIATELY | KMU_PUSH_USAGE_MASK_DESTROY_AFTER;
	const size_t key_size = READ_ONLY_KEY_SIZE;

	LOG_DBG("Test revokable not supported");
	err = test_import_key(key_id, persistence, usage_flags, test_key, key_size);
	zassert_equal(err, PSA_ERROR_NOT_SUPPORTED,
		      "Read-only key: Import should fail with PSA_ERROR_NOT_SUPPORTED, err: %d", err);
}

void test_main(void)
{
	clear_kmu_slots();

	test_rotatable_import();

	test_rotatable_import_push();

	test_rotatable_import_push_destroy();

	test_revokable_import_not_supported();

	test_read_only_import_not_supported();
}
