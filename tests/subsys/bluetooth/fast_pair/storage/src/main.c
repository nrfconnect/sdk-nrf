/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <zephyr/settings/settings.h>

#include "fp_storage.h"
#include "fp_crypto.h"

#include "storage_mock.h"

#define ACCOUNT_KEY_MAX_CNT	CONFIG_BT_FAST_PAIR_STORAGE_ACCOUNT_KEY_MAX
#define ACCOUNT_KEY_LEN		FP_CRYPTO_ACCOUNT_KEY_LEN


static void generate_account_key(uint8_t key_id, uint8_t *account_key)
{
        memset(account_key, key_id, ACCOUNT_KEY_LEN);
}

static bool check_account_key_id(uint8_t key_id, const uint8_t *account_key)
{
	if (account_key[0] != key_id) {
		return false;
	}

	for (size_t i = 1; i < ACCOUNT_KEY_LEN; i++) {
		zassert_equal(account_key[i], key_id, "Invalid Account Key");
	}

	return true;
}

static bool account_key_find_cb(const uint8_t *account_key, void *context)
{
	uint8_t *key_id = context;

	return check_account_key_id(*key_id, account_key);
}

static void validate_unloaded(void)
{
	int err;
	uint8_t all_keys[ACCOUNT_KEY_MAX_CNT][ACCOUNT_KEY_LEN];
	size_t key_count = ACCOUNT_KEY_MAX_CNT;
	uint8_t key_id = 0;

	err = fp_storage_account_key_count();
	zassert_equal(err, -ENODATA, "Expected error before settings load");

	err = fp_storage_account_keys_get(all_keys, &key_count);
	zassert_equal(err, -ENODATA, "Expected error before settings load");

	err = fp_storage_account_key_find(all_keys[0], account_key_find_cb, &key_id);
	zassert_equal(err, -ENODATA, "Expected error before settings load");
}

static void reload_keys_from_storage(void)
{
	int err;

	fp_storage_ram_clear();
	validate_unloaded();
	err = settings_load();

	zassert_ok(err, "Failed to load settings");
}

static void setup_fn(void)
{
	int err;

	validate_unloaded();

	/* Load empty settings before the test to initialize fp_storage. */
	err = settings_load();

	zassert_ok(err, "Settings load failed");
}

static void teardown_fn(void)
{
	fp_storage_ram_clear();
	storage_mock_clear();
	validate_unloaded();
}

static void test_unloaded(void)
{
	static const uint8_t test_key_id = 0;

	int err;
	uint8_t account_key[ACCOUNT_KEY_LEN];

	validate_unloaded();
	generate_account_key(test_key_id, account_key);

	err = fp_storage_account_key_save(account_key);
	zassert_equal(err, -ENODATA, "Expected error before settings load");
}

static void test_one_key(void)
{
	static const uint8_t test_key_id = 5;

	int err;
	uint8_t account_key[ACCOUNT_KEY_LEN];

	generate_account_key(test_key_id, account_key);
	err = fp_storage_account_key_save(account_key);
	zassert_ok(err, "Unexpected error during Account Key save");

	uint8_t read_keys[ACCOUNT_KEY_MAX_CNT][ACCOUNT_KEY_LEN];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_account_key_count(), 1, "Invalid Account Key count");
	err = fp_storage_account_keys_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(check_account_key_id(test_key_id, read_keys[0]), "Invalid key on read");

	/* Reload keys from storage and validate them again. */
	reload_keys_from_storage();
	memset(read_keys, 0, sizeof(read_keys));
	read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_account_key_count(), 1, "Invalid Account Key count");
	err = fp_storage_account_keys_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(check_account_key_id(test_key_id, read_keys[0]), "Invalid key on read");
}

static void test_duplicate(void)
{
	static const uint8_t test_key_id = 3;

	int err;
	uint8_t account_key[ACCOUNT_KEY_LEN];

	generate_account_key(test_key_id, account_key);
	err = fp_storage_account_key_save(account_key);
	zassert_ok(err, "Unexpected error during Account Key save");

	/* Try to add key duplicate. */
	err = fp_storage_account_key_save(account_key);
	zassert_ok(err, "Unexpected error during Account Key save");

	uint8_t read_keys[ACCOUNT_KEY_MAX_CNT][ACCOUNT_KEY_LEN];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_account_key_count(), 1, "Invalid Account Key count");
	err = fp_storage_account_keys_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(check_account_key_id(test_key_id, read_keys[0]), "Invalid key on read");

	/* Reload keys from storage and validate them again. */
	reload_keys_from_storage();
	memset(read_keys, 0, sizeof(read_keys));
	read_cnt = ACCOUNT_KEY_MAX_CNT;

	zassert_equal(fp_storage_account_key_count(), 1, "Invalid Account Key count");
	err = fp_storage_account_keys_get(read_keys, &read_cnt);
	zassert_ok(err, "Getting Account Keys failed");
	zassert_equal(read_cnt, 1, "Invalid Account Key count");
	zassert_true(check_account_key_id(test_key_id, read_keys[0]), "Invalid key on read");
}

static void generate_and_store_keys(uint8_t gen_count)
{
	int err;
	uint8_t account_key[ACCOUNT_KEY_LEN];

	for (uint8_t i = 0; i < gen_count; i++) {
		generate_account_key(i, account_key);

		err = fp_storage_account_key_save(account_key);
		zassert_ok(err, "Failed to store Account Key");
	}
}

static void test_invalid_calls(void)
{
	static const uint8_t test_key_cnt = 3;

	int err;
	uint8_t found_key[ACCOUNT_KEY_LEN];
	uint8_t read_keys[ACCOUNT_KEY_MAX_CNT][ACCOUNT_KEY_LEN];

	generate_and_store_keys(test_key_cnt);

	for (size_t read_cnt = 0; read_cnt <= ACCOUNT_KEY_MAX_CNT; read_cnt++) {
		size_t read_cnt_ret = read_cnt;

		err = fp_storage_account_keys_get(read_keys, &read_cnt_ret);

		if (read_cnt < test_key_cnt) {
			zassert_equal(err, -EINVAL, "Expected error when array is too small");
		} else {
			zassert_ok(err, "Unexpected error during Account Keys get");
			zassert_equal(read_cnt_ret, test_key_cnt, "Inavlid account key count");

			for (size_t i = 0; i < read_cnt_ret; i++) {
				zassert_true(check_account_key_id(i, read_keys[i]),
					     "Invalid key on read");
			}
		}
	}

	uint8_t key_id = 0;

	err = fp_storage_account_key_find(found_key, NULL, &key_id);
	zassert_equal(err, -EINVAL, "Expected error when callback is NULL");
}

static void test_find(void)
{
	static const size_t test_key_cnt = 3;
	int err = 0;

	generate_and_store_keys(test_key_cnt);

	for (uint8_t key_id = 0; key_id < test_key_cnt; key_id++) {
		uint8_t account_key[ACCOUNT_KEY_LEN];
		uint8_t prev_key_id = key_id;

		err = fp_storage_account_key_find(account_key, account_key_find_cb, &key_id);
		zassert_ok(err, "Failed to find Account Key");
		zassert_equal(prev_key_id, key_id, "Context should not be modified");
		zassert_true(check_account_key_id(key_id, account_key), "Found wrong Account Key");
	}

	for (uint8_t key_id = 0; key_id < (test_key_cnt + 1); key_id++) {
		uint8_t prev_key_id = key_id;

		err = fp_storage_account_key_find(NULL, account_key_find_cb, &key_id);
		zassert_equal(prev_key_id, key_id, "Context should not be modified");
		if (err) {
			zassert_equal(key_id, test_key_cnt, "Failed to find Acount Key");
			break;
		}
	}

	zassert_equal(err, -ESRCH, "Expected error when key cannot be found");
}

static void loop_validate_keys(uint8_t stored_cnt)
{
	int res;

	int key_cnt;
	uint8_t read_keys[ACCOUNT_KEY_MAX_CNT][ACCOUNT_KEY_LEN];
	size_t read_cnt = ACCOUNT_KEY_MAX_CNT;

	key_cnt = fp_storage_account_key_count();

	res = fp_storage_account_keys_get(read_keys, &read_cnt);
	zassert_ok(res, "Getting Account Keys failed");
	zassert_equal(key_cnt, read_cnt, "Invalid key count");
	zassert_equal(key_cnt, MIN(stored_cnt, ACCOUNT_KEY_MAX_CNT), "Invalid key count");

	size_t key_id_min = (stored_cnt < ACCOUNT_KEY_MAX_CNT) ?
			    (0) : (stored_cnt - ACCOUNT_KEY_MAX_CNT);
	size_t key_id_max = stored_cnt - 1;

	for (size_t i = key_id_min; i <= key_id_max; i++) {
		bool found = false;

		for (size_t j = 0; j < read_cnt; j++) {
			if (found) {
				zassert_false(check_account_key_id(i, read_keys[j]),
					      "Key duplicate found");
			}

			if (check_account_key_id(i, read_keys[j])) {
				found = true;
			}
		}

		zassert_true(found, "Key dropped by the Fast Pair storage");
	}
}

static void test_loop(void)
{
	for (uint8_t i = 1; i < UCHAR_MAX; i++) {
		setup_fn();

		generate_and_store_keys(i);
		loop_validate_keys(i);

		/* Reload keys from storage and validate them again. */
		reload_keys_from_storage();
		loop_validate_keys(i);

		teardown_fn();
	}
}

void test_main(void)
{
	ztest_test_suite(fast_pair_storage_tests,
			 ztest_unit_test(test_unloaded),
			 ztest_unit_test_setup_teardown(test_one_key, setup_fn, teardown_fn),
			 ztest_unit_test_setup_teardown(test_duplicate, setup_fn, teardown_fn),
			 ztest_unit_test_setup_teardown(test_invalid_calls, setup_fn, teardown_fn),
			 ztest_unit_test_setup_teardown(test_find, setup_fn, teardown_fn),
			 ztest_unit_test(test_loop)
			 );

	ztest_run_test_suite(fast_pair_storage_tests);
}
