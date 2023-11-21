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
static uint8_t account_key_metadata[ACCOUNT_KEY_CNT];
static uint8_t account_key_count;

static uint8_t account_key_order[ACCOUNT_KEY_CNT];

static int settings_set_err;
static bool reset_prepare;
static atomic_t settings_loaded = ATOMIC_INIT(false);

static int fp_settings_load_ak(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	uint8_t id;
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

	id = ACCOUNT_KEY_METADATA_FIELD_GET(data.account_key_metadata, ID);
	if ((id < ACCOUNT_KEY_MIN_ID) || (id > ACCOUNT_KEY_MAX_ID)) {
		return -EINVAL;
	}

	index = account_key_id_to_idx(id);
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

	if (ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[index], ID) != 0) {
		return -EINVAL;
	}

	account_key_list[index] = data.account_key;
	account_key_metadata[index] = data.account_key_metadata;

	return 0;
}

static int fp_settings_load_ak_order(size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	if (len != sizeof(account_key_order)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, account_key_order, len);
	if (rc < 0) {
		return rc;
	}

	if (rc != len) {
		return -EINVAL;
	}

	return 0;
}

static int fp_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err = 0;

	if (!strncmp(name, SETTINGS_AK_NAME_PREFIX, sizeof(SETTINGS_AK_NAME_PREFIX) - 1)) {
		err = fp_settings_load_ak(name, len, read_cb, cb_arg);
	} else if (!strncmp(name, SETTINGS_AK_ORDER_KEY_NAME, sizeof(SETTINGS_AK_ORDER_KEY_NAME))) {
		err = fp_settings_load_ak_order(len, read_cb, cb_arg);
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

static uint8_t bump_ak_id(uint8_t id)
{
	__ASSERT_NO_MSG((id >= ACCOUNT_KEY_MIN_ID) && (id <= ACCOUNT_KEY_MAX_ID));

	return (id < ACCOUNT_KEY_MIN_ID + ACCOUNT_KEY_CNT) ? (id + ACCOUNT_KEY_CNT) :
							     (id - ACCOUNT_KEY_CNT);
}

static uint8_t get_least_recent_key_id(void)
{
	/* The function can be used only if the Account Key List is full. */
	__ASSERT_NO_MSG(account_key_count == ACCOUNT_KEY_CNT);

	return account_key_order[ACCOUNT_KEY_CNT - 1];
}

static uint8_t next_account_key_id(void)
{
	if (account_key_count < ACCOUNT_KEY_CNT) {
		return ACCOUNT_KEY_MIN_ID + account_key_count;
	}

	return bump_ak_id(get_least_recent_key_id());
}

static void ak_order_update_ram(uint8_t used_id)
{
	bool id_found = false;
	size_t found_idx;

	for (size_t i = 0; i < account_key_count; i++) {
		if (account_key_order[i] == used_id) {
			id_found = true;
			found_idx = i;
			break;
		}
	}

	if (id_found) {
		for (size_t i = found_idx; i > 0; i--) {
			account_key_order[i] = account_key_order[i - 1];
		}
	} else {
		for (size_t i = account_key_count - 1; i > 0; i--) {
			account_key_order[i] = account_key_order[i - 1];
		}
	}
	account_key_order[0] = used_id;
}

static int commit_ak_order(void)
{
	int err;
	size_t ak_order_update_count = 0;

	for (size_t i = account_key_count; i < ACCOUNT_KEY_CNT; i++) {
		if (account_key_order[i] != 0) {
			return -EINVAL;
		}
	}

	for (int i = 0; i < account_key_count; i++) {
		uint8_t id = ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[i], ID);
		bool id_found = false;

		for (size_t j = 0; j < account_key_count; j++) {
			if (account_key_order[j] == id) {
				id_found = true;
				break;
			}
		}
		if (!id_found) {
			if (ak_order_update_count >= account_key_count) {
				__ASSERT_NO_MSG(false);
				return -EINVAL;
			}
			ak_order_update_ram(id);
			ak_order_update_count++;
			/* Start loop again to check whether after update no existent AK has been
			 * lost. Set iterator to -1 to start next iteration with iterator equal to 0
			 * after i++ operation.
			 */
			i = -1;
		}
	}

	if (ak_order_update_count > 0) {
		err = settings_save_one(SETTINGS_AK_ORDER_FULL_NAME, account_key_order,
					sizeof(account_key_order));
		if (err) {
			return err;
		}
	}

	return 0;
}

static int fp_settings_commit(void)
{
	uint8_t id;
	int first_zero_idx = -1;
	int err;

	if (reset_prepare) {
		/* The module expects to be reset by Fast Pair storage manager. */
		return 0;
	}

	if (settings_set_err) {
		return settings_set_err;
	}

	for (size_t i = 0; i < ACCOUNT_KEY_CNT; i++) {
		id = ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[i], ID);
		if (id == 0) {
			first_zero_idx = i;
			break;
		}

		if (account_key_id_to_idx(id) != i) {
			return -EINVAL;
		}
	}

	if (first_zero_idx != -1) {
		for (size_t i = 0; i < first_zero_idx; i++) {
			id = ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[i], ID);
			if (id != ACCOUNT_KEY_MIN_ID + i) {
				return -EINVAL;
			}
		}

		for (size_t i = first_zero_idx + 1; i < ACCOUNT_KEY_CNT; i++) {
			id = ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[i], ID);
			if (id != 0) {
				return -EINVAL;
			}
		}

		account_key_count = first_zero_idx;
	} else {
		account_key_count = ACCOUNT_KEY_CNT;
	}

	err = commit_ak_order();
	if (err) {
		return err;
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
			int err;

			ak_order_update_ram(ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[i],
									   ID));
			err = settings_save_one(SETTINGS_AK_ORDER_FULL_NAME, account_key_order,
						sizeof(account_key_order));
			if (err) {
				LOG_ERR("Unable to save new Account Key order in Settings. "
					"Not propagating the error and keeping updated Account Key "
					"order in RAM. After the Settings error the Account Key "
					"order may change at reboot.");
			}

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

	uint8_t id;
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
		LOG_INF("Account Key List full - erasing the least recently used Account Key.");
	}

	id = next_account_key_id();
	index = account_key_id_to_idx(id);

	data.account_key_metadata = 0;
	ACCOUNT_KEY_METADATA_FIELD_SET(data.account_key_metadata, ID, id);
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
	account_key_metadata[index] = data.account_key_metadata;

	if (account_key_count < ACCOUNT_KEY_CNT) {
		account_key_count++;
	}

	ak_order_update_ram(id);
	err = settings_save_one(SETTINGS_AK_ORDER_FULL_NAME, account_key_order,
				sizeof(account_key_order));
	if (err) {
		LOG_ERR("Unable to save new Account Key order in Settings. "
			"Not propagating the error and keeping updated Account Key "
			"order in RAM. After the Settings error the Account Key "
			"order may change at reboot.");
	}

	return 0;
}

void fp_storage_ak_ram_clear(void)
{
	memset(account_key_list, 0, sizeof(account_key_list));
	memset(account_key_metadata, 0, sizeof(account_key_metadata));
	account_key_count = 0;

	memset(account_key_order, 0, sizeof(account_key_order));

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
	atomic_set(&settings_loaded, true);
}

static int fp_storage_ak_delete(uint8_t index)
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
	int err;

	for (uint8_t index = 0; index < ACCOUNT_KEY_CNT; index++) {
		err = fp_storage_ak_delete(index);
		if (err) {
			return err;
		}
	}

	err = settings_delete(SETTINGS_AK_ORDER_FULL_NAME);
	if (err) {
		return err;
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
