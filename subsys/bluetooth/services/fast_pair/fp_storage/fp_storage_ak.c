/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/settings/settings.h>
#include <bluetooth/services/fast_pair.h>

#include "fp_common.h"
#include "fp_storage_ak.h"
#include "fp_storage_ak_priv.h"
#include "fp_storage_manager.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

static struct fp_account_key account_key_list[ACCOUNT_KEY_CNT];
static uint8_t account_key_loaded_ids[ACCOUNT_KEY_CNT];
static uint8_t account_key_next_id;
static uint8_t account_key_count;

static int settings_set_err;
static bool reset_prepare;
static atomic_t settings_loaded = ATOMIC_INIT(false);

static int fp_settings_load_ak(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	uint8_t index;
	struct account_key_data data;
	const char *name_suffix;
	size_t name_suffix_len;

	if (len != sizeof(data)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &data, sizeof(data));
	if (rc < 0) {
		return rc;
	}

	if (rc != sizeof(data)) {
		return -EINVAL;
	}

	if ((data.account_key_id < ACCOUNT_KEY_MIN_ID) ||
	    (data.account_key_id > ACCOUNT_KEY_MAX_ID)) {
		return -EINVAL;
	}

	index = account_key_id_to_idx(data.account_key_id);
	name_suffix = &name[sizeof(SETTINGS_AK_NAME_PREFIX) - 1];
	name_suffix_len = strlen(name_suffix);

	if ((name_suffix_len < 1) || (name_suffix_len > SETTINGS_AK_NAME_MAX_SUFFIX_LEN)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < strlen(name_suffix); i++) {
		if (!isdigit(name_suffix[i])) {
			return -EINVAL;
		}
	}

	if (index != atoi(name_suffix)) {
		return -EINVAL;
	}

	if (account_key_loaded_ids[index] != 0) {
		return -EINVAL;
	}

	account_key_list[index] = data.account_key;
	account_key_loaded_ids[index] = data.account_key_id;

	return 0;
}

static int fp_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err = 0;
	const char *key = SETTINGS_AK_NAME_PREFIX;

	if (!strncmp(name, key, sizeof(SETTINGS_AK_NAME_PREFIX) - 1)) {
		err = fp_settings_load_ak(name, len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be propagated by the commit callback.
	 * Errors returned in the settings set callback are not propagated further.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return err;
}

static bool zero_id_check(uint8_t start_idx)
{
	for (size_t i = start_idx; i < ACCOUNT_KEY_CNT; i++) {
		if (account_key_loaded_ids[i] != 0) {
			return false;
		}
	}

	return true;
}

static bool id_sequence_check(uint8_t start_idx)
{
	__ASSERT_NO_MSG(start_idx > 0);
	for (size_t i = start_idx; i < ACCOUNT_KEY_CNT; i++) {
		if (account_key_loaded_ids[i] !=
		    next_account_key_id(account_key_loaded_ids[i - 1])) {
			return false;
		}
	}

	return true;
}

static bool rollover_check(uint8_t cur_id, uint8_t prev_id)
{
	return ((cur_id == (prev_id - (ACCOUNT_KEY_CNT - 1))) ||
		(cur_id == (prev_id + (ACCOUNT_KEY_CNT + 1))));
}

static int fp_settings_commit(void)
{
	uint8_t prev_id;
	uint8_t cur_id;
	int first_zero_idx = -1;
	int rollover_idx = -1;

	if (reset_prepare) {
		/* The module expects to be reset by Fast Pair storage manager. */
		return 0;
	}

	if (settings_set_err) {
		return settings_set_err;
	}

	cur_id = account_key_loaded_ids[0];
	if ((cur_id != 0) && (account_key_id_to_idx(cur_id) != 0)) {
		return -EINVAL;
	}

	if (cur_id == 0) {
		first_zero_idx = 0;
	}

	for (size_t i = 1; (i < ACCOUNT_KEY_CNT) &&
			   (first_zero_idx == -1); i++) {
		prev_id = cur_id;
		cur_id = account_key_loaded_ids[i];
		if (cur_id == 0) {
			if (prev_id != (i - 1 + ACCOUNT_KEY_MIN_ID)) {
				return -EINVAL;
			}

			first_zero_idx = i;
			break;
		}
		if (cur_id != next_account_key_id(prev_id)) {
			if (!rollover_check(cur_id, prev_id)) {
				return -EINVAL;
			}

			rollover_idx = i;
			break;
		}
	}

	if (first_zero_idx != -1) {
		if (!zero_id_check(first_zero_idx + 1)) {
			return -EINVAL;
		}

		account_key_count = first_zero_idx;
		account_key_next_id = first_zero_idx + ACCOUNT_KEY_MIN_ID;
	} else if (rollover_idx != -1) {
		if (!id_sequence_check(rollover_idx + 1)) {
			return -EINVAL;
		}

		account_key_count = ACCOUNT_KEY_CNT;
		account_key_next_id = next_account_key_id(account_key_loaded_ids[rollover_idx - 1]);
	} else {
		account_key_count = ACCOUNT_KEY_CNT;
		account_key_next_id = next_account_key_id(
					account_key_loaded_ids[ACCOUNT_KEY_CNT - 1]);
	}

	atomic_set(&settings_loaded, true);

	return 0;
}

int fp_storage_ak_count(void)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	return account_key_count;
}

int fp_storage_ak_get(struct fp_account_key *buf, size_t *key_count)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	if (*key_count < account_key_count) {
		return -EINVAL;
	}

	memcpy(buf, account_key_list, account_key_count * sizeof(account_key_list[0]));
	*key_count = account_key_count;

	return 0;
}

int fp_storage_ak_find(struct fp_account_key *account_key,
		       fp_storage_ak_check_cb account_key_check_cb, void *context)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	if (!account_key_check_cb) {
		return -EINVAL;
	}

	for (size_t i = 0; i < account_key_count; i++) {
		if (account_key_check_cb(&account_key_list[i], context)) {
			if (account_key) {
				*account_key = account_key_list[i];
			}

			return 0;
		}
	}

	return -ESRCH;
}

static int ak_name_gen(char *name, uint8_t index)
{
	int n;

	n = snprintf(name, SETTINGS_AK_NAME_MAX_SIZE, "%s%u", SETTINGS_AK_FULL_PREFIX, index);
	__ASSERT_NO_MSG(n < SETTINGS_AK_NAME_MAX_SIZE);
	if (n < 0) {
		return n;
	}

	return 0;
}

int fp_storage_ak_save(const struct fp_account_key *account_key)
{
	if (!atomic_get(&settings_loaded)) {
		return -ENODATA;
	}

	uint8_t index;
	struct account_key_data data;
	char name[SETTINGS_AK_NAME_MAX_SIZE];
	int err;

	for (size_t i = 0; i < account_key_count; i++) {
		if (!memcmp(account_key->key, account_key_list[i].key, FP_ACCOUNT_KEY_LEN)) {
			LOG_INF("Account Key already saved - skipping.");
			return 0;
		}
	}
	if (account_key_count == ACCOUNT_KEY_CNT) {
		LOG_INF("Account Key List full - erasing the oldest Account Key.");
	}

	index = account_key_id_to_idx(account_key_next_id);

	data.account_key_id = account_key_next_id;
	data.account_key = *account_key;

	err = ak_name_gen(name, index);
	if (err) {
		return err;
	}

	err = settings_save_one(name, &data, sizeof(data));
	if (err) {
		return err;
	}

	account_key_list[index] = *account_key;

	account_key_next_id = next_account_key_id(account_key_next_id);

	if (account_key_count < ACCOUNT_KEY_CNT) {
		account_key_count++;
	}

	return 0;
}

void fp_storage_ak_ram_clear(void)
{
	memset(account_key_list, 0, sizeof(account_key_list));
	memset(account_key_loaded_ids, 0, sizeof(account_key_loaded_ids));
	account_key_next_id = 0;
	account_key_count = 0;

	settings_set_err = 0;
	atomic_set(&settings_loaded, false);
	reset_prepare = false;
}

bool bt_fast_pair_has_account_key(void)
{
	return (fp_storage_ak_count() > 0);
}

static void fp_storage_ak_empty_init(void)
{
	account_key_next_id = ACCOUNT_KEY_MIN_ID;
	atomic_set(&settings_loaded, true);
}

int fp_storage_ak_delete(uint8_t index)
{
	int err;
	char name[SETTINGS_AK_NAME_MAX_SIZE];

	err = ak_name_gen(name, index);
	if (err) {
		return err;
	}

	err = settings_delete(name);
	if (err) {
		return err;
	}

	return 0;
}

static int fp_storage_ak_reset(void)
{
	for (uint8_t index = 0; index < ACCOUNT_KEY_CNT; index++) {
		int err;

		err = fp_storage_ak_delete(index);
		if (err) {
			return err;
		}
	}

	fp_storage_ak_ram_clear();

	fp_storage_ak_empty_init();

	return 0;
}

static void reset_prepare_set(void)
{
	atomic_set(&settings_loaded, false);
	reset_prepare = true;
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_ak, SETTINGS_AK_SUBTREE_NAME, NULL, fp_settings_set,
			       fp_settings_commit, NULL);

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_storage_ak, fp_storage_ak_reset, reset_prepare_set);
