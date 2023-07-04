/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/settings/settings.h>

#include "fp_storage_ak.h"
#include "fp_storage_ak_priv.h"
#include "fp_common.h"

#include "storage_mock.h"
#include "common_utils.h"

#define ACCOUNT_KEY_MAX_CNT	CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX


static void reload_keys_from_storage(void)
{
	int err;

	fp_storage_ak_ram_clear();
	cu_account_keys_validate_unloaded();
	err = settings_load();

	zassert_ok(err, "Failed to load settings");
}

static void before_fn(void *f)
{
	ARG_UNUSED(f);

	int err;

	cu_account_keys_validate_unloaded();

	/* Load empty settings before the test to initialize fp_storage. */
	err = settings_load();

	zassert_ok(err, "Settings load failed");
}

static void after_fn(void *f)
{
	ARG_UNUSED(f);

	fp_storage_ak_ram_clear();
	storage_mock_clear();
	cu_account_keys_validate_unloaded();
}

static bool unloaded_ak_check_cb(const struct fp_account_key *account_key, void *context)
{
	zassert_unreachable("Callback should never be called");
	return false;
}

ZTEST(suite_fast_pair_storage, test_unloaded)
{
	static const uint8_t seed = 0;

	int err;
	struct fp_account_key account_key;
	struct fp_account_key read_keys[ACCOUNT_KEY_MAX_CNT];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	/* Check that Account Key storage operations fail when settings are unloaded. */
	after_fn(NULL);

	cu_generate_account_key(seed, &account_key);
	err = fp_storage_ak_save(&account_key);
	zassert_equal(err, -ENODATA, "Expected error before settings load");

	err = fp_storage_ak_count();
	zassert_equal(err, -ENODATA, "Expected error before settings load");

	err = fp_storage_ak_get(read_keys, &read_cnt);
	zassert_equal(err, -ENODATA, "Expected error before settings load");

	err = fp_storage_ak_find(NULL, unloaded_ak_check_cb, NULL);
	zassert_equal(err, -ENODATA, "Expected error before settings load");
}

ZTEST(suite_fast_pair_storage, test_one_key)
{
	static const uint8_t seed = 5;

	int err;
	struct fp_account_key account_key;

	cu_generate_account_key(seed, &account_key);
	err = fp_storage_ak_save(&account_key);
	zassert_ok(err, "Unexpected error during Account Key save");

	struct fp_account_key read_keys[ACCOUNT_KEY_MAX_CNT];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_ak_count(), 1, "Invalid Account Key count");
	err = fp_storage_ak_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(cu_check_account_key_seed(seed, &read_keys[0]), "Invalid key on read");

	/* Reload keys from storage and validate them again. */
	reload_keys_from_storage();
	memset(read_keys, 0, sizeof(read_keys));
	read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_ak_count(), 1, "Invalid Account Key count");
	err = fp_storage_ak_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(cu_check_account_key_seed(seed, &read_keys[0]), "Invalid key on read");
}

ZTEST(suite_fast_pair_storage, test_duplicate)
{
	static const uint8_t seed = 3;

	int err;
	struct fp_account_key account_key;

	cu_generate_account_key(seed, &account_key);
	err = fp_storage_ak_save(&account_key);
	zassert_ok(err, "Unexpected error during Account Key save");

	/* Try to add key duplicate. */
	err = fp_storage_ak_save(&account_key);
	zassert_ok(err, "Unexpected error during Account Key save");

	struct fp_account_key read_keys[ACCOUNT_KEY_MAX_CNT];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_ak_count(), 1, "Invalid Account Key count");
	err = fp_storage_ak_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(cu_check_account_key_seed(seed, &read_keys[0]), "Invalid key on read");

	/* Reload keys from storage and validate them again. */
	reload_keys_from_storage();
	memset(read_keys, 0, sizeof(read_keys));
	read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_ak_count(), 1, "Invalid Account Key count");
	err = fp_storage_ak_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(cu_check_account_key_seed(seed, &read_keys[0]), "Invalid key on read");
}

ZTEST(suite_fast_pair_storage, test_invalid_calls)
{
	static const uint8_t first_seed = 3;
	static const uint8_t test_key_cnt = 3;

	int err;
	struct fp_account_key found_key;
	struct fp_account_key read_keys[ACCOUNT_KEY_MAX_CNT];

	cu_account_keys_generate_and_store(first_seed, test_key_cnt);

	for (size_t read_cnt = 0; read_cnt <= ACCOUNT_KEY_MAX_CNT; read_cnt++) {
		size_t read_cnt_ret = read_cnt;

		err = fp_storage_ak_get(read_keys, &read_cnt_ret);

		if (read_cnt < test_key_cnt) {
			zassert_equal(err, -EINVAL, "Expected error when array is too small");
		} else {
			zassert_ok(err, "Unexpected error during Account Keys get");
			zassert_equal(read_cnt_ret, test_key_cnt, "Inavlid account key count");

			for (size_t i = 0; i < read_cnt_ret; i++) {
				zassert_true(cu_check_account_key_seed(
						first_seed + i, &read_keys[i]),
					     "Invalid key on read");
			}
		}
	}

	uint8_t find_seed = first_seed;

	err = fp_storage_ak_find(&found_key, NULL, &find_seed);
	zassert_equal(err, -EINVAL, "Expected error when callback is NULL");
}

static bool account_key_find_cb(const struct fp_account_key *account_key, void *context)
{
	uint8_t *seed = context;

	return cu_check_account_key_seed(*seed, account_key);
}

ZTEST(suite_fast_pair_storage, test_find)
{
	static const uint8_t first_seed = 0;
	static const size_t test_key_cnt = 3;
	int err = 0;

	cu_account_keys_generate_and_store(first_seed, test_key_cnt);

	for (uint8_t seed = first_seed; seed < (first_seed + test_key_cnt); seed++) {
		struct fp_account_key account_key;
		uint8_t prev_seed = seed;

		err = fp_storage_ak_find(&account_key, account_key_find_cb, &seed);
		zassert_ok(err, "Failed to find Account Key");
		zassert_equal(prev_seed, seed, "Context should not be modified");
		zassert_true(cu_check_account_key_seed(seed, &account_key),
			     "Found wrong Account Key");
	}

	for (uint8_t seed = first_seed; seed < (first_seed + test_key_cnt + 1); seed++) {
		uint8_t prev_seed = seed;

		err = fp_storage_ak_find(NULL, account_key_find_cb, &seed);
		zassert_equal(prev_seed, seed, "Context should not be modified");
		if (err) {
			zassert_equal(seed, (first_seed + test_key_cnt),
				      "Failed to find Acount Key");
			break;
		}
	}

	zassert_equal(err, -ESRCH, "Expected error when key cannot be found");
}

ZTEST(suite_fast_pair_storage, test_loop)
{
	static const uint8_t first_seed = 0;

	for (uint8_t i = 1; i < UCHAR_MAX; i++) {
		/* Triggering before function twice in a row would lead to assertion failure.
		 * The function checks if settings are not loaded and then loads the settings.
		 */
		if (i > 1) {
			before_fn(NULL);
		}

		cu_account_keys_generate_and_store(first_seed, i);
		cu_account_keys_validate_loaded(first_seed, i);

		/* Reload keys from storage and validate them again. */
		reload_keys_from_storage();
		cu_account_keys_validate_loaded(first_seed, i);

		after_fn(NULL);
	}
}


ZTEST_SUITE(suite_fast_pair_storage, NULL, NULL, before_fn, after_fn, NULL);
