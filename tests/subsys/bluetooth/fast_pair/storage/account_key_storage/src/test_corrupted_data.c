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

BUILD_ASSERT((ACCOUNT_KEY_CNT == 1) ||
	     !IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_OWNER_ACCOUNT_KEY),
	     "This test suite does not support storage configuration with the Owner Account Key "
	     "and standard Account Keys");

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

static void store_key(uint8_t key_id, bool is_owner)
{
	int err;
	char settings_key[SETTINGS_AK_NAME_MAX_SIZE] = SETTINGS_AK_FULL_PREFIX;
	struct account_key_data data;

	zassert_true(('0' + account_key_id_to_idx(key_id)) <= '9', "Key index out of range");
	settings_key[ARRAY_SIZE(settings_key) - 2] = ('0' + account_key_id_to_idx(key_id));

	data.account_key_metadata = 0;
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, ID, key_id);
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_OWNER_ACCOUNT_KEY)
	    && is_owner) {
		ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, OWNER, 1);
	}
	cu_generate_account_key(key_id, &data.account_key);

	err = settings_save_one(settings_key, &data, sizeof(data));
	zassert_ok(err, "Unexpected error in settings save");
}

static void self_test_after(void *f)
{
	fp_storage_ak_ram_clear();
	fp_storage_manager_ram_clear();
	storage_mock_clear();
	cu_account_keys_validate_uninitialized();
}

ZTEST(suite_settings_mock_self_test, test_self_basic)
{
	int err;
	uint8_t key_id = ACCOUNT_KEY_MIN_ID;
	uint8_t key_cnt = 0;

	for (size_t i = 0; i < ACCOUNT_KEY_CNT; i++) {
		if (i == 0) {
			store_key(key_id, true);
		} else {
			store_key(key_id, false);
		}
		key_cnt++;
		key_id = next_account_key_id(key_id);
	}


	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");

	err = fp_storage_init();
	zassert_ok(err, "Unexpected error in modules init");

	cu_account_keys_validate_loaded(ACCOUNT_KEY_MIN_ID, key_cnt);
}

ZTEST(suite_settings_mock_self_test, test_self_rollover)
{
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_OWNER_ACCOUNT_KEY)) {
		ztest_test_skip();
	}

	int err;
	uint8_t key_id = ACCOUNT_KEY_MIN_ID;
	uint8_t key_cnt = 0;

	for (size_t i = 0; i < ACCOUNT_KEY_CNT; i++) {
		if (i == 0) {
			store_key(key_id, true);
		} else {
			store_key(key_id, false);
		}
		key_cnt++;
		key_id = next_account_key_id(key_id);
	}

	store_key(key_id, false);
	key_cnt++;


	err = settings_load();
	zassert_ok(err, "Unexpected error in settings load");

	err = fp_storage_init();
	zassert_ok(err, "Unexpected error in modules init");

	cu_account_keys_validate_loaded(ACCOUNT_KEY_MIN_ID, key_cnt);
}

ZTEST_SUITE(suite_settings_mock_self_test, NULL, NULL, NULL, self_test_after, NULL);


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
	if (ACCOUNT_KEY_CNT < 2) {
		store_key(ACCOUNT_KEY_MIN_ID + 1, false);
	} else {
		store_key(next_account_key_id(ACCOUNT_KEY_MIN_ID), false);
	}

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_drop_key)
{
	if (ACCOUNT_KEY_CNT < 2) {
		ztest_test_skip();
	}

	uint8_t key_id = ACCOUNT_KEY_MIN_ID;

	store_key(key_id, true);

	key_id = next_account_key_id(key_id);
	store_key(key_id, false);

	key_id = next_account_key_id(key_id);
	key_id = next_account_key_id(key_id);
	store_key(key_id, false);

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_drop_keys)
{
	if (ACCOUNT_KEY_CNT < 2) {
		ztest_test_skip();
	}

	uint8_t key_id = ACCOUNT_KEY_MIN_ID;

	store_key(key_id, true);

	for (size_t i = 0; i < ACCOUNT_KEY_CNT; i++) {
		key_id = next_account_key_id(key_id);
	}

	zassert_equal(account_key_id_to_idx(ACCOUNT_KEY_MIN_ID), account_key_id_to_idx(key_id),
		      "Test should write key exactly after rollover");
	store_key(key_id, false);

	initialization_error_validate();
}

ZTEST(suite_fast_pair_storage_corrupted, test_no_account_key_metadata_owner)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_OWNER_ACCOUNT_KEY)) {
		ztest_test_skip();
	} else {
		static const uint8_t key_id = ACCOUNT_KEY_MIN_ID;

		store_key(key_id, false);

		initialization_error_validate();
	}
}

ZTEST_SUITE(suite_fast_pair_storage_corrupted, NULL, NULL, before_fn, after_fn, NULL);
