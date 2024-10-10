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
#include <zephyr/settings/settings.h>
#include <bluetooth/services/fast_pair/fast_pair.h>

#include "fp_common.h"
#include "fp_storage_ak.h"
#include "fp_storage_ak_bond.h"
#include "fp_storage_ak_priv.h"
#include "fp_storage_manager.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

#define FP_BOND_INFO_AK_ID_UNKNOWN	(0)
BUILD_ASSERT(FP_BOND_INFO_AK_ID_UNKNOWN == (ACCOUNT_KEY_MIN_ID - 1));

/* Non-volatile bond information that is stored in the Settings subsystem. */
struct fp_bond_info_nv {
	uint8_t account_key_metadata_id;
	bt_addr_le_t addr;
};

struct fp_bond_info {
	const void *conn_ctx;
	bool bonded;
	struct fp_bond_info_nv nv;
};

static const struct fp_storage_ak_bond_bt_request_cb *bt_request_cb;
static struct fp_bond_info fp_bonds[FP_BONDS_ARRAY_LEN];

#define FP_BONDS_FOREACH(_iterator)				\
	for (struct fp_bond_info *_iterator = fp_bonds;		\
	     _iterator <= &fp_bonds[ARRAY_SIZE(fp_bonds) - 1]; _iterator++)

static struct fp_account_key account_key_list[ACCOUNT_KEY_CNT];
static uint8_t account_key_metadata[ACCOUNT_KEY_CNT];
static uint8_t account_key_count;

static uint8_t account_key_order[ACCOUNT_KEY_CNT];

static int settings_set_err;
static bool is_enabled;

static int fp_settings_data_read(void *data, size_t data_len,
				 size_t read_len, settings_read_cb read_cb, void *cb_arg)
{
	int rc;

	if (read_len != data_len) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, data, data_len);
	if (rc < 0) {
		return rc;
	}

	if (rc != data_len) {
		return -EINVAL;
	}

	return 0;
}

static ssize_t index_from_settings_name_get(const char *name_suffix, size_t max_suffix_len)
{
	size_t name_suffix_len;

	name_suffix_len = strlen(name_suffix);

	if ((name_suffix_len < 1) || (name_suffix_len > max_suffix_len)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < name_suffix_len; i++) {
		if (!isdigit(name_suffix[i])) {
			return -EINVAL;
		}
	}

	return atoi(name_suffix);
}

static int fp_settings_load_ak(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err;
	uint8_t id;
	uint8_t index;
	struct account_key_data data;
	ssize_t name_index;

	err = fp_settings_data_read(&data, sizeof(data), len, read_cb, cb_arg);
	if (err) {
		return err;
	}

	id = ACCOUNT_KEY_METADATA_FIELD_GET(data.account_key_metadata, ID);
	if ((id < ACCOUNT_KEY_MIN_ID) || (id > ACCOUNT_KEY_MAX_ID)) {
		return -EINVAL;
	}

	index = account_key_id_to_idx(id);
	name_index = index_from_settings_name_get(&name[sizeof(SETTINGS_AK_NAME_PREFIX) - 1],
						  SETTINGS_AK_NAME_MAX_SUFFIX_LEN);
	if ((name_index < 0) || (index != name_index)) {
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
	return fp_settings_data_read(account_key_order, sizeof(account_key_order), len, read_cb,
				     cb_arg);
}

static int fp_settings_load_bond(const char *name, size_t len, settings_read_cb read_cb,
				 void *cb_arg)
{
	int err;
	ssize_t index;
	struct fp_bond_info_nv bond_nv;

	err = fp_settings_data_read(&bond_nv, sizeof(bond_nv), len, read_cb, cb_arg);
	if (err) {
		return err;
	}

	index = index_from_settings_name_get(&name[sizeof(SETTINGS_BOND_INFO_NAME_PREFIX) - 1],
					     SETTINGS_BOND_INFO_NAME_MAX_SUFFIX_LEN);
	if ((index < 0) || (index >= ARRAY_SIZE(fp_bonds))) {
		return -EINVAL;
	}

	fp_bonds[index].nv = bond_nv;

	LOG_DBG("Bond loaded successfully");
	return 0;
}

static int fp_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	int err = 0;

	if (!strncmp(name, SETTINGS_AK_NAME_PREFIX, sizeof(SETTINGS_AK_NAME_PREFIX) - 1)) {
		err = fp_settings_load_ak(name, len, read_cb, cb_arg);
	} else if (!strncmp(name, SETTINGS_AK_ORDER_KEY_NAME, sizeof(SETTINGS_AK_ORDER_KEY_NAME))) {
		err = fp_settings_load_ak_order(len, read_cb, cb_arg);
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND) &&
		   !strncmp(name, SETTINGS_BOND_INFO_NAME_PREFIX,
		   sizeof(SETTINGS_BOND_INFO_NAME_PREFIX) - 1)) {
		err = fp_settings_load_bond(name, len, read_cb, cb_arg);
	} else {
		err = -ENOENT;
	}

	/* The first reported settings set error will be remembered by the module.
	 * The error will then be returned when calling fp_storage_ak_init.
	 */
	if (err && !settings_set_err) {
		settings_set_err = err;
	}

	return 0;
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

static int fp_settings_validate_ak(void)
{
	uint8_t id;
	int first_zero_idx = -1;

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

	return 0;
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

static int fp_settings_validate_ak_order(void)
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

static int fp_settings_suffixed_name_gen(char *name, size_t name_max_size, const char *name_prefix,
					 uint8_t index)
{
	int n;

	n = snprintf(name, name_max_size, "%s%u", name_prefix, index);
	__ASSERT_NO_MSG(n < name_max_size);
	if (n < 0) {
		return n;
	}

	return 0;
}

static int ak_name_gen(char *name, size_t name_len, uint8_t index)
{
	__ASSERT_NO_MSG(name_len == SETTINGS_AK_NAME_MAX_SIZE);
	return fp_settings_suffixed_name_gen(name, name_len, SETTINGS_AK_FULL_PREFIX, index);
}

static int fp_bond_info_name_gen(char *name, size_t name_len, uint8_t index)
{
	__ASSERT_NO_MSG(name_len == SETTINGS_BOND_INFO_NAME_MAX_SIZE);
	return fp_settings_suffixed_name_gen(name, name_len, SETTINGS_BOND_INFO_FULL_PREFIX, index);
}

static size_t fp_bond_info_idx_get(struct fp_bond_info *bond)
{
	__ASSERT_NO_MSG((bond >= fp_bonds) && (bond <= &fp_bonds[ARRAY_SIZE(fp_bonds) - 1]));
	return bond - fp_bonds;
}

static int fp_bond_info_settings_delete(struct fp_bond_info *bond)
{
	int err;
	char name[SETTINGS_BOND_INFO_NAME_MAX_SIZE];

	err = fp_bond_info_name_gen(name, sizeof(name), fp_bond_info_idx_get(bond));
	if (err) {
		return err;
	}

	err = settings_delete(name);
	if (err) {
		return err;
	}

	return 0;
}

static int bond_remove(struct fp_bond_info *bond, bool bt_stack_bond_remove)
{
	int err;

	__ASSERT_NO_MSG(!bt_addr_le_eq(&bond->nv.addr, BT_ADDR_LE_ANY));

	if (bt_stack_bond_remove) {
		__ASSERT_NO_MSG(bt_request_cb);

		err = bt_request_cb->bond_remove(&bond->nv.addr);
		if (err) {
			if (err == -ESRCH) {
				LOG_DBG("Bond not found in the Bluetooth stack. "
					"Continuing the removal operation");
			} else {
				LOG_ERR("Failed to remove Bluetooth bond (err %d)", err);
				return err;
			}
		}
	}

	/* Bond entry might have been deleted earlier by the fp_storage_ak_bond_delete. */
	if (!bt_addr_le_eq(&bond->nv.addr, BT_ADDR_LE_ANY)) {
		err = fp_bond_info_settings_delete(bond);
		if (err) {
			LOG_ERR("Failed to delete Fast Pair bond from Settings (err %d)", err);
			return err;
		}

		*bond = (struct fp_bond_info){0};
	}

	return 0;
}

static int fp_bond_info_duplicates_handle(void)
{
	FP_BONDS_FOREACH(bond) {
		bool duplicate_found = false;
		bt_addr_le_t addr = bond->nv.addr;
		int err;

		if (bt_addr_le_eq(&bond->nv.addr, BT_ADDR_LE_ANY)) {
			continue;
		}

		for (struct fp_bond_info *duplicate = bond + 1;
		     duplicate <= &fp_bonds[ARRAY_SIZE(fp_bonds) - 1]; duplicate++) {
			if (bt_addr_le_eq(&duplicate->nv.addr, &addr)) {
				/* Error during Fast Pair Procedure. */
				LOG_DBG("Removing duplicated bond entries at bootup");
				if (!duplicate_found) {
					duplicate_found = true;
					err = bond_remove(duplicate, true);
				} else {
					err = bond_remove(duplicate, false);
				}
				if (err) {
					LOG_ERR("bond_remove failed (err %d)", err);
					return err;
				}
			}
		}

		/* The module that handles Bluetooth bond request callbacks calls
		 * fp_storage_ak_bond_delete to remove fp_bond_info when a Bluetooth bond is
		 * deleted for any reason. Because of this, the fp_bond_info might have already
		 * been cleared at this point.
		 */
		if (duplicate_found && bt_addr_le_eq(&bond->nv.addr, &addr)) {
			err = bond_remove(bond, false);
			if (err) {
				LOG_ERR("bond_remove failed (err %d)", err);
				return err;
			}
		}
	}

	return 0;
}

static int invalid_bonds_purge(bool skip_ongoing_procedures)
{
	FP_BONDS_FOREACH(bond) {
		int err;
		uint8_t ak_idx;

		if (bt_addr_le_eq(&bond->nv.addr, BT_ADDR_LE_ANY)) {
			continue;
		}

		if (!bond->bonded ||
		    (bond->nv.account_key_metadata_id == FP_BOND_INFO_AK_ID_UNKNOWN)) {
			if (!skip_ongoing_procedures) {
				LOG_DBG("Removing invalid Fast Pair bond");
				err = bond_remove(bond, true);
				if (err) {
					LOG_ERR("bond_remove failed (err %d)", err);
					return err;
				}
			}

			continue;
		}

		ak_idx = account_key_id_to_idx(bond->nv.account_key_metadata_id);
		if (bond->nv.account_key_metadata_id !=
		    ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[ak_idx], ID)) {
			LOG_DBG("Removing bond associated with invalid Account Key");
			err = bond_remove(bond, true);
			if (err) {
				LOG_ERR("bond_remove failed (err %d)", err);
				return err;
			}
		}
	}

	return 0;
}

static int fp_settings_validate_fp_bond_info(void)
{
	/* Assert that zero-initialized addresses are equal to BT_ADDR_LE_ANY. */
	static const bt_addr_le_t addr;
	int err;

	__ASSERT_NO_MSG(bt_addr_le_eq(&addr, BT_ADDR_LE_ANY));
	ARG_UNUSED(addr);

	/* Duplicates must be removed before purging invalid bonds. */
	err = fp_bond_info_duplicates_handle();
	if (err) {
		return err;
	}

	err = invalid_bonds_purge(false);
	if (err) {
		return err;
	}

	return 0;
}

static int fp_settings_validate(void)
{
	int err;

	if (settings_set_err) {
		return settings_set_err;
	}

	err = fp_settings_validate_ak();
	if (err) {
		return err;
	}

	err = fp_settings_validate_ak_order();
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		err = fp_settings_validate_fp_bond_info();
		if (err) {
			return err;
		}
	}

	return 0;
}

int fp_storage_ak_count(void)
{
	if (!is_enabled) {
		return -EACCES;
	}

	return account_key_count;
}

int fp_storage_ak_get(struct fp_account_key *buf, size_t *key_count)
{
	if (!is_enabled) {
		return -EACCES;
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
	if (!is_enabled) {
		return -EACCES;
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

static struct fp_bond_info *fp_bond_info_by_conn_get(const void *conn_ctx)
{
	FP_BONDS_FOREACH(bond) {
		if (bond->conn_ctx == conn_ctx) {
			return bond;
		}
	}

	return NULL;
}

static struct fp_bond_info *fp_bond_info_free_get(void)
{
	FP_BONDS_FOREACH(bond) {
		if (bt_addr_le_eq(&bond->nv.addr, BT_ADDR_LE_ANY)) {
			return bond;
		}
	}

	return NULL;
}

static struct fp_bond_info *fp_bond_info_by_addr_bonded_get(const bt_addr_le_t *addr)
{
	FP_BONDS_FOREACH(bond) {
		if (bond->bonded && bt_addr_le_eq(&bond->nv.addr, addr)) {
			return bond;
		}
	}

	return NULL;
}

static int fp_bond_info_settings_save(struct fp_bond_info *bond,
				      const struct fp_bond_info *bond_rollback_copy)
{
	int err;
	char name[SETTINGS_BOND_INFO_NAME_MAX_SIZE];

	err = fp_bond_info_name_gen(name, sizeof(name), fp_bond_info_idx_get(bond));
	if (err) {
		*bond = *bond_rollback_copy;
		return err;
	}

	err = settings_save_one(name, &bond->nv, sizeof(bond->nv));
	if (err) {
		*bond = *bond_rollback_copy;
		return err;
	}

	return 0;
}

int fp_storage_ak_save(const struct fp_account_key *account_key, const void *conn_ctx)
{
	if (!is_enabled) {
		return -EACCES;
	}

	uint8_t id;
	uint8_t index;
	struct account_key_data data;
	char name[SETTINGS_AK_NAME_MAX_SIZE];
	int err;

	struct fp_bond_info *bond;
	bool ak_overwritten = false;

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

	err = ak_name_gen(name, sizeof(name), index);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		struct fp_bond_info bond_rollback_copy;

		__ASSERT_NO_MSG(conn_ctx);

		bond = fp_bond_info_by_conn_get(conn_ctx);
		if (!bond) {
			return -EINVAL;
		}

		__ASSERT(bond->bonded, "An fp_bond writing an Account Key must be already bonded");

		bond_rollback_copy = *bond;

		bond->nv.account_key_metadata_id = id;

		err = fp_bond_info_settings_save(bond, &bond_rollback_copy);
		if (err) {
			return err;
		}
	} else {
		ARG_UNUSED(conn_ctx);
	}

	err = settings_save_one(name, &data, sizeof(data));
	if (err) {
		return err;
	}

	account_key_list[index] = *account_key;
	account_key_metadata[index] = data.account_key_metadata;

	if (account_key_count < ACCOUNT_KEY_CNT) {
		account_key_count++;
	} else {
		ak_overwritten = true;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		/* Procedure finished successfully. Setting conn_ctx to NULL. */
		bond->conn_ctx = NULL;
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

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND) && ak_overwritten) {
		/* Account Key overwritten. Remove bonds related with overwritten Account Key. */
		err = invalid_bonds_purge(true);
		if (err) {
			LOG_ERR("invalid_bonds_purge failed (err %d). Ignoring the error and "
				"continuing. There might be some invalid bonds present in the "
				"Fast Pair AK storage subsystem.", err);
		}
	}

	return 0;
}

static int ak_id_get(uint8_t *id, const struct fp_account_key *account_key)
{
	for (size_t i = 0; i < account_key_count; i++) {
		if (!memcmp(account_key_list[i].key, account_key->key, FP_ACCOUNT_KEY_LEN)) {
			*id = ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[i], ID);
			return 0;
		}
	}

	return -ESRCH;
}

void fp_storage_ak_bond_bt_request_cb_register(const struct fp_storage_ak_bond_bt_request_cb *cb)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	__ASSERT_NO_MSG(cb && cb->bond_remove && cb->is_addr_bonded);

	bt_request_cb = cb;
}

int fp_storage_ak_bond_conn_create(const void *conn_ctx, const bt_addr_le_t *addr,
				   const struct fp_account_key *account_key)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		__ASSERT_NO_MSG(false);
		return -ENOTSUP;
	}

	if (!is_enabled) {
		return -EACCES;
	}

	int err;
	uint8_t id;
	struct fp_bond_info *bond;
	struct fp_bond_info bond_rollback_copy = (struct fp_bond_info){0};

	if (account_key) {
		err = ak_id_get(&id, account_key);
		if (err) {
			return err;
		}
	} else {
		/* The actual ID will be determined during Account Key write. */
		id = FP_BOND_INFO_AK_ID_UNKNOWN;
	}

	bond = fp_bond_info_free_get();
	if (!bond) {
		return -ENOMEM;
	}

	bond->conn_ctx = conn_ctx;
	bond->bonded = false;
	bond->nv.account_key_metadata_id = id;
	bond->nv.addr = *addr;

	err = fp_bond_info_settings_save(bond, &bond_rollback_copy);
	if (err) {
		return err;
	}

	return 0;
}

void fp_storage_ak_bond_conn_confirm(const void *conn_ctx, const bt_addr_le_t *addr)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	if (!is_enabled) {
		return;
	}

	struct fp_bond_info *bond_confirmed;
	int err;

	__ASSERT_NO_MSG(bt_addr_le_is_identity(addr));

	bond_confirmed = fp_bond_info_by_conn_get(conn_ctx);
	if (!bond_confirmed) {
		LOG_ERR("Bond not found in Fast Pair storage");
		return;
	}
	__ASSERT_NO_MSG(bt_addr_le_eq(&bond_confirmed->nv.addr, addr));

	bond_confirmed->bonded = true;

	/* Remove bonded duplicates that were overwritten by new bond. */
	FP_BONDS_FOREACH(bond) {
		if ((bond->conn_ctx != conn_ctx) && bond->bonded &&
		    bt_addr_le_eq(&bond->nv.addr, addr)) {
			/* Only one bonded duplicate can be present here. */
			err = bond_remove(bond, false);
			if (err) {
				LOG_ERR("bond_remove failed (err %d)", err);
			} else {
				LOG_DBG("Removed duplicated bond");
			}

			break;
		}
	}

	if (bond_confirmed->nv.account_key_metadata_id != FP_BOND_INFO_AK_ID_UNKNOWN) {
		/* Account Key already associated (subsequent pairing procedure).
		 * Procedure finished successfully. Setting conn_ctx to NULL.
		 */
		bond_confirmed->conn_ctx = NULL;
	}
}

void fp_storage_ak_bond_conn_addr_update(const void *conn_ctx, const bt_addr_le_t *new_addr)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	if (!is_enabled) {
		return;
	}

	int err;
	struct fp_bond_info *bond;
	struct fp_bond_info bond_rollback_copy;

	bond = fp_bond_info_by_conn_get(conn_ctx);
	if (!bond) {
		return;
	}
	bond_rollback_copy = *bond;

	bond->nv.addr = *new_addr;

	err = fp_bond_info_settings_save(bond, &bond_rollback_copy);
	if (err) {
		return;
	}
}

void fp_storage_ak_bond_conn_finalize(const void *conn_ctx)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	if (!is_enabled) {
		return;
	}

	struct fp_bond_info *bond;
	int err;

	bond = fp_bond_info_by_conn_get(conn_ctx);
	if (!bond) {
		return;
	}

	/* If at this point the conn_ctx is not set to NULL, the procedure is incomplete. */
	err = bond_remove(bond, true);
	if (err) {
		LOG_ERR("bond_remove failed (err %d)", err);
	}
}

void fp_storage_ak_bond_delete(const bt_addr_le_t *addr)
{
	if (!IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		__ASSERT_NO_MSG(false);
		return;
	}

	if (!is_enabled) {
		return;
	}

	LOG_DBG("Bluetooth bond deleted");
	FP_BONDS_FOREACH(unused) {
		struct fp_bond_info *bond;
		int err;

		/* In some edge cases, while enabling and validating bonds there might be more than
		 * one bonded bond present.
		 */
		bond = fp_bond_info_by_addr_bonded_get(addr);
		if (!bond) {
			break;
		}

		err = bond_remove(bond, false);
		if (err) {
			LOG_ERR("bond_remove failed (err %d)", err);
		}
	}
}

void fp_storage_ak_ram_clear(void)
{
	memset(account_key_list, 0, sizeof(account_key_list));
	memset(account_key_metadata, 0, sizeof(account_key_metadata));
	account_key_count = 0;

	memset(account_key_order, 0, sizeof(account_key_order));

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		memset(fp_bonds, 0, sizeof(fp_bonds));
		bt_request_cb = NULL;
	}

	settings_set_err = 0;

	is_enabled = false;
}

bool fp_storage_ak_has_account_key(void)
{
	return (fp_storage_ak_count() > 0);
}

static int fp_storage_ak_init(void)
{
	int err;

	if (is_enabled) {
		LOG_WRN("fp_storage_ak module already initialized");
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		__ASSERT_NO_MSG(bt_request_cb);
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		FP_BONDS_FOREACH(bond) {
			/* If Fast Pair was disabled before the peer that performed the unsuccessful
			 * Fast Pair Procedure disconnected, there may be a conn_ctx not set to
			 * NULL. All conn_ctx should be set to NULL at this point.
			 */
			bond->conn_ctx = NULL;

			bond->bonded = bt_request_cb->is_addr_bonded(&bond->nv.addr);
			if (bond->bonded) {
				LOG_DBG("Loaded bond has been found in the Bluetooth bond list");
			}
		}
	}

	err = fp_settings_validate();
	if (err) {
		return err;
	}

	is_enabled = true;

	return 0;
}

static int fp_storage_ak_uninit(void)
{
	if (!is_enabled) {
		LOG_WRN("fp_storage_ak module already uninitialized");
		return 0;
	}

	is_enabled = false;

	return 0;
}

static int fp_storage_ak_delete(uint8_t index)
{
	int err;
	char name[SETTINGS_AK_NAME_MAX_SIZE];

	err = ak_name_gen(name, sizeof(name), index);
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
	bool was_enabled = is_enabled;
	const struct fp_storage_ak_bond_bt_request_cb *registered_bt_request_cb;

	if (was_enabled) {
		err = fp_storage_ak_uninit();
		if (err) {
			return err;
		}
	}

	for (uint8_t index = 0; index < ACCOUNT_KEY_CNT; index++) {
		err = fp_storage_ak_delete(index);
		if (err) {
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		FP_BONDS_FOREACH(bond) {
			if (bt_addr_le_eq(&bond->nv.addr, BT_ADDR_LE_ANY)) {
				continue;
			}

			err = bond_remove(bond, true);
			if (err) {
				LOG_ERR("bond_remove failed (err %d)", err);
			}
		}
	}

	err = settings_delete(SETTINGS_AK_ORDER_FULL_NAME);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		registered_bt_request_cb = bt_request_cb;
	}

	fp_storage_ak_ram_clear();

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_STORAGE_AK_BOND)) {
		bt_request_cb = registered_bt_request_cb;
	}

	if (was_enabled) {
		err = fp_storage_ak_init();
		if (err) {
			return err;
		}
	}

	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_ak, SETTINGS_AK_SUBTREE_NAME, NULL, fp_settings_set,
			       NULL, NULL);

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_storage_ak, fp_storage_ak_reset,
				   fp_storage_ak_init, fp_storage_ak_uninit);
