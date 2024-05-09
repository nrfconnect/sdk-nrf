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
#include "fp_common.h"

#include "storage_mock.h"
#include "common_utils.h"

#define ACCOUNT_KEY_MAX_CNT	CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX


static void reload_keys_from_storage(void)
{
	int err;

	fp_storage_ak_ram_clear();
	fp_storage_manager_ram_clear();
	cu_account_keys_validate_uninitialized();
	err = settings_load();
	zassert_ok(err, "Failed to load settings");

	err = fp_storage_init();
	zassert_ok(err, "Failed to initialize module");
}

static void before_fn(void *f)
{
	ARG_UNUSED(f);

	int err;

	cu_account_keys_validate_uninitialized();

	/* Load empty settings before the test to initialize fp_storage. */
	err = settings_load();
	zassert_ok(err, "Settings load failed");

	err = fp_storage_init();
	zassert_ok(err, "Failed to initialize module");
}

static void after_fn(void *f)
{
	ARG_UNUSED(f);

	fp_storage_ak_ram_clear();
	fp_storage_manager_ram_clear();
	storage_mock_clear();
	cu_account_keys_validate_uninitialized();
}

ZTEST(suite_fast_pair_storage_common, test_settings_unloaded)
{
	int err;

	/* Check that Account Key storage operations fail when settings are unloaded. */
	after_fn(NULL);

	err = fp_storage_init();
	zassert_not_equal(err, 0, "Expected error before settings load");

	cu_account_keys_validate_uninitialized();
}

ZTEST(suite_fast_pair_storage_common, test_uninitialized)
{
	int err;

	/* Check that Account Key storage operations fail when fp_storage is uninitialized */
	after_fn(NULL);

	err = settings_load();
	zassert_ok(err, "Settings load failed");

	cu_account_keys_validate_uninitialized();
}

ZTEST(suite_fast_pair_storage_common, test_one_key)
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

ZTEST(suite_fast_pair_storage_common, test_reinitialization)
{
	static const uint8_t first_seed;
	static const size_t test_key_cnt = MIN(ACCOUNT_KEY_MAX_CNT, 5);
	int err;

	cu_account_keys_generate_and_store(first_seed, test_key_cnt);
	cu_account_keys_validate_loaded(first_seed, test_key_cnt);

	err = fp_storage_uninit();
	zassert_ok(err, "Uninitialization failed");
	cu_account_keys_validate_uninitialized();

	err = fp_storage_init();
	zassert_ok(err, "Initialization failed");
	cu_account_keys_validate_loaded(first_seed, test_key_cnt);
}

ZTEST(suite_fast_pair_storage_common, test_duplicate)
{
	static const uint8_t seed = 3;

	int err;
	struct fp_account_key account_key;

	cu_generate_account_key(seed, &account_key);
	err = fp_storage_ak_save(&account_key);
	zassert_ok(err, "Unexpected error during Account Key save");

	/* Try to add key duplicate. */
	err = fp_storage_ak_save(&account_key);
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_OWNER_ACCOUNT_KEY)) {
		zassert_ok(err, "Unexpected error during Account Key save");
	} else {
		zassert_equal(err, -EALREADY, "Error expected if second key added");
	}

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

ZTEST(suite_fast_pair_storage_common, test_invalid_calls)
{
	static const uint8_t first_seed = 3;
	static const uint8_t test_key_cnt = MIN(ACCOUNT_KEY_MAX_CNT, 3);

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

ZTEST(suite_fast_pair_storage_common, test_find)
{
	static const uint8_t first_seed;
	static const size_t test_key_cnt = MIN(ACCOUNT_KEY_MAX_CNT, 3);
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

ZTEST(suite_fast_pair_storage_common, test_bt_has_ak)
{
	static const uint8_t first_seed;
	static const size_t test_key_cnt = MIN(ACCOUNT_KEY_MAX_CNT, 3);

	zassert_false(fp_storage_ak_has_account_key(), "No account key should be stored");

	cu_account_keys_generate_and_store(first_seed, test_key_cnt);
	zassert_true(fp_storage_ak_has_account_key(), "Account key should be stored");

	/* Reload keys from storage and validate them again. */
	reload_keys_from_storage();
	zassert_true(fp_storage_ak_has_account_key(), "Account key should be stored after reload");
}

ZTEST(suite_fast_pair_storage_common, test_loop)
{
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_OWNER_ACCOUNT_KEY) &&
	   (ACCOUNT_KEY_CNT < 2)) {
		ztest_test_skip();
	}

	static const uint8_t first_seed;

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

ZTEST(suite_fast_pair_storage_common, test_owner_key)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_OWNER_ACCOUNT_KEY)) {
		ztest_test_skip();
	} else {
		static const uint8_t seed;

		int ret;
		struct fp_account_key account_key;
		struct fp_account_key next_account_key;

		cu_generate_account_key(seed, &account_key);
		ret = fp_storage_ak_is_owner(&account_key);
		zassert_equal(ret, -ESRCH, "No owner account key should be stored");

		ret = fp_storage_ak_save(&account_key);
		zassert_ok(ret, "Unexpected error during Account Key save");

		ret = fp_storage_ak_is_owner(&account_key);
		zassert_equal(ret, 1, "Owner account key should be stored");

		cu_generate_account_key(seed + 1, &next_account_key);
		ret = fp_storage_ak_is_owner(&next_account_key);
		zassert_equal(ret, 0, "Owner account key different than questioned");

		/* Reload keys from storage and validate them again. */
		reload_keys_from_storage();

		ret = fp_storage_ak_is_owner(&account_key);
		zassert_equal(ret, 1, "Owner account key should be stored");

		ret = fp_storage_ak_is_owner(&next_account_key);
		zassert_equal(ret, 0, "Owner account key different than questioned");
	}
}

ZTEST_SUITE(suite_fast_pair_storage_common, NULL, NULL, before_fn, after_fn, NULL);
