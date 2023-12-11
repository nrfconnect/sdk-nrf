/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/settings/settings.h>

#include "fp_storage_ak.h"
#include "fp_storage.h"
#include "fp_storage_ak_priv.h"
#include "fp_storage_manager_priv.h"

#include "storage_mock.h"
#include "common_utils.h"

/* This function is made just for this corrupted data test.
 * It assumes that the least recently used key is always the oldest key that has been written.
 * It's not meant to be used anywhere else.
 */
static uint8_t next_account_key_id(uint8_t account_key_id)
{
	if (account_key_id == ACCOUNT_KEY_MAX_ID) {
		return ACCOUNT_KEY_MIN_ID;
	}

	return account_key_id + 1;
}

static void store_key(uint8_t key_id)
{
	int err;
	char settings_key[SETTINGS_AK_NAME_MAX_SIZE] = SETTINGS_AK_FULL_PREFIX;
	struct account_key_data data;

	zassert_true(('0' + account_key_id_to_idx(key_id)) <= '9', "Key index out of range");
	settings_key[ARRAY_SIZE(settings_key) - 2] = ('0' + account_key_id_to_idx(key_id));

	data.account_key_metadata = 0;
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, ID, key_id);
	cu_generate_account_key(key_id, &data.account_key);

	err = settings_save_one(settings_key, &data, sizeof(data));
	zassert_ok(err, "Unexpected error in settings save");
}

static void self_test_after(void)
{
	fp_storage_ak_ram_clear();
	fp_storage_manager_ram_clear();
	storage_mock_clear();
	cu_account_keys_validate_uninitialized();
}

static void self_test(void)
{
	int err;
	uint8_t key_id = ACCOUNT_KEY_MIN_ID;
	uint8_t key_cnt = 0;

	store_key(key_id);
	key_cnt++;
	key_id = next_account_key_id(key_id);

	store_key(key_id);
	key_cnt++;
	key_id = next_account_key_id(key_id);

	store_key(key_id);
	key_cnt++;

	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");

	err = fp_storage_init();
	zassert_ok(err, "Unexpected error in modules init");

	cu_account_keys_validate_loaded(ACCOUNT_KEY_MIN_ID, key_cnt);
}

static void self_test_rollover(void)
{
	int err;
	uint8_t key_id = ACCOUNT_KEY_MIN_ID;
	uint8_t key_cnt = 0;

	for (size_t i = 0; i < ACCOUNT_KEY_CNT; i++) {
		store_key(key_id);
		key_cnt++;
		key_id = next_account_key_id(key_id);
	}

	store_key(key_id);
	key_cnt++;
	key_id = next_account_key_id(key_id);

	store_key(key_id);
	key_cnt++;

	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");

	err = fp_storage_init();
	zassert_ok(err, "Unexpected error in modules init");

	cu_account_keys_validate_loaded(ACCOUNT_KEY_MIN_ID, key_cnt);
}

static void *setup_fn(void)
{
	/* Perform simple self tests to ensure that the test method is valid. */
	self_test();
	self_test_after();

	self_test_rollover();
	self_test_after();

	return NULL;
}

static void before_fn(void *f)
{
	/* Make sure that settings are not yet loaded. Settings will be populated with data and
	 * loaded by the unit test.
	 */
	ARG_UNUSED(f);

	cu_account_keys_validate_uninitialized();
}

static void after_fn(void *f)
{
	ARG_UNUSED(f);

	fp_storage_ak_ram_clear();
	fp_storage_manager_ram_clear();
	storage_mock_clear();
}

static void initialization_error_validate(void)
{
	int err;

	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");

	err = fp_storage_init();
	zassert_not_equal(err, 0, "Expected error during initialization");
	cu_account_keys_validate_uninitialized();
}

ZTEST(suite_fast_pair_storage_corrupted, test_wrong_settings_key)
{
	static const uint8_t key_id = ACCOUNT_KEY_MIN_ID;

	int err;
	struct account_key_data data;

	data.account_key_metadata = 0;
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, ID, key_id);
	cu_generate_account_key(key_id, &data.account_key);

	err = settings_save_one(SETTINGS_AK_SUBTREE_NAME SETTINGS_NAME_SEPARATOR_STR
				"not_account_key", &data, sizeof(data));
	zassert_ok(err, "Unexpected error in settings save");

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_wrong_data_len)
{
	static const uint8_t key_id = ACCOUNT_KEY_MIN_ID;

	int err;
	char settings_key[SETTINGS_AK_NAME_MAX_SIZE] = SETTINGS_AK_FULL_PREFIX;
	struct account_key_data data;

	zassert_true(('0' + account_key_id_to_idx(key_id)) <= '9', "Key index out of range");
	settings_key[ARRAY_SIZE(settings_key) - 2] = ('0' + account_key_id_to_idx(key_id));

	data.account_key_metadata = 0;
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, ID, key_id);
	cu_generate_account_key(key_id, &data.account_key);

	err = settings_save_one(settings_key, &data, sizeof(data) - 1);
	zassert_ok(err, "Unexpected error in settings save");

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_inconsistent_key_id)
{
	static const uint8_t key_id = ACCOUNT_KEY_MIN_ID;

	int err;
	char settings_key[SETTINGS_AK_NAME_MAX_SIZE] = SETTINGS_AK_FULL_PREFIX;
	struct account_key_data data;

	zassert_true(('0' + account_key_id_to_idx(key_id)) <= '9', "Key index out of range");
	settings_key[ARRAY_SIZE(settings_key) - 2] = ('0' + account_key_id_to_idx(key_id));

	data.account_key_metadata = 0;
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, ID, next_account_key_id(key_id));
	cu_generate_account_key(key_id, &data.account_key);

	err = settings_save_one(settings_key, &data, sizeof(data));
	zassert_ok(err, "Unexpected error in settings save");

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_drop_first_key)
{
	store_key(next_account_key_id(ACCOUNT_KEY_MIN_ID));

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_drop_key)
{
	uint8_t key_id = ACCOUNT_KEY_MIN_ID;

	store_key(key_id);

	key_id = next_account_key_id(key_id);
	store_key(key_id);

	key_id = next_account_key_id(key_id);
	key_id = next_account_key_id(key_id);
	store_key(key_id);

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_drop_keys)
{
	uint8_t key_id = ACCOUNT_KEY_MIN_ID;

	store_key(key_id);

	for (size_t i = 0; i < ACCOUNT_KEY_CNT; i++) {
		key_id = next_account_key_id(key_id);
	}

	zassert_equal(account_key_id_to_idx(ACCOUNT_KEY_MIN_ID), account_key_id_to_idx(key_id),
		      "Test should write key exactly after rollover");
	store_key(key_id);

	initialization_error_validate();
}

ZTEST_SUITE(suite_fast_pair_storage_corrupted, NULL, setup_fn, before_fn, after_fn, NULL);
