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
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/settings/settings.h>
#include <bluetooth/services/fast_pair/fast_pair.h>

#include "fp_common.h"
#include "fp_storage_ak.h"
#include "fp_storage_ak_priv.h"
#include "fp_storage_manager.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_storage, CONFIG_FP_STORAGE_LOG_LEVEL);

/* Non-volatile bond information that is stored in the Settings subsystem. */
struct fp_bond_info_nv {
	uint8_t account_key_metadata_id;
	uint8_t identity;
	bt_addr_le_t addr;
};

struct fp_bond {
	const struct bt_conn *conn;
	bool bonded;
	struct fp_bond_info_nv nv;
};

static struct fp_account_key account_key_list[ACCOUNT_KEY_CNT];
static uint8_t account_key_metadata[ACCOUNT_KEY_CNT];
static uint8_t account_key_count;

static uint8_t account_key_order[ACCOUNT_KEY_CNT];

static struct fp_bond fp_bonds[CONFIG_BT_MAX_PAIRED + CONFIG_BT_MAX_CONN];

static int settings_set_err;
static bool is_enabled;

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

static int fp_settings_load_bond(const char *name, size_t len, settings_read_cb read_cb,
				 void *cb_arg)
{
	int rc;
	uint8_t index;
	struct fp_bond_info_nv bond_nv;
	const char *name_suffix;
	size_t name_suffix_len;

	if (len != sizeof(bond_nv)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &bond_nv, sizeof(bond_nv));
	if (rc < 0) {
		return rc;
	}

	if (rc != sizeof(bond_nv)) {
		return -EINVAL;
	}

	name_suffix = &name[sizeof(SETTINGS_BOND_NAME_PREFIX) - 1];
	name_suffix_len = strlen(name_suffix);

	if ((name_suffix_len < 1) || (name_suffix_len > SETTINGS_BOND_NAME_MAX_SUFFIX_LEN)) {
		return -EINVAL;
	}

	for (size_t i = 0; i < strlen(name_suffix); i++) {
		if (!isdigit(name_suffix[i])) {
			return -EINVAL;
		}
	}

	index = atoi(name_suffix);
	if (index >= ARRAY_SIZE(fp_bonds)) {
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
	} else if (!strncmp(name, SETTINGS_BOND_NAME_PREFIX,
		   sizeof(SETTINGS_BOND_NAME_PREFIX) - 1)) {
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

static int validate_ak_order(void)
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

static int bond_name_gen(char *name, uint8_t index)
{
	int n;

	n = snprintf(name, SETTINGS_BOND_NAME_MAX_SIZE, "%s%u", SETTINGS_BOND_FULL_PREFIX, index);
	__ASSERT_NO_MSG(n < SETTINGS_BOND_NAME_MAX_SIZE);
	if (n < 0) {
		return n;
	}

	return 0;
}

static int fp_settings_bond_delete(size_t bond_idx)
{
	int err;
	char name[SETTINGS_BOND_NAME_MAX_SIZE];

	err = bond_name_gen(name, bond_idx);
	if (err) {
		return err;
	}

	err = settings_delete(name);
	if (err) {
		return err;
	}

	return 0;
}

static int bond_delete(size_t bond_idx)
{
	int err;

	err = fp_settings_bond_delete(bond_idx);
	if (err) {
		return err;
	}

	fp_bonds[bond_idx] = (struct fp_bond){0};

	return 0;
}

int bond_remove(size_t bond_idx)
{
	int err;

	if (fp_bonds[bond_idx].bonded) {
		/* Bond entry will be deleted in the bond_deleted callback triggered
		 * by calling bt_unpair.
		 */
		err = bt_unpair(fp_bonds[bond_idx].nv.identity, &fp_bonds[bond_idx].nv.addr);
		if (err) {
			LOG_ERR("Failed to unpair (err %d)", err);
			return err;
		}
	} else {
		err = bond_delete(bond_idx);
		if (err) {
			LOG_ERR("Failed to delete bond (err %d)", err);
			return err;
		}
	}

	LOG_DBG("Bond removed successfully");

	return 0;
}

static void bond_remove_old_id(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		int err;
		bool account_key_found = false;

		if (bt_addr_le_eq(&fp_bonds[i].nv.addr, BT_ADDR_LE_ANY) || !fp_bonds[i].bonded ||
		    (fp_bonds[i].nv.account_key_metadata_id == 0)) {
			continue;
		}

		for (size_t j = 0; j < account_key_count; j++) {
			if (fp_bonds[i].nv.account_key_metadata_id ==
			    ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[j], ID)) {
				account_key_found = true;
				break;
			}
		}

		if (!account_key_found) {
			LOG_DBG("Removing bond associated with old Account Key");
			err = bond_remove(i);
			if (err) {
				LOG_ERR("Failed to remove bond (err %d)", err);
			}
		}
	}
}

static void bonded_field_set(const struct bt_bond_info *info, void *user_data)
{
	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		if (bt_addr_le_eq(&fp_bonds[i].nv.addr, &info->addr)) {
			LOG_DBG("Loaded bond has been found in the Bluetooth bond list");
			fp_bonds[i].bonded = true;
		}
	}
}

static void handle_bond_duplicates(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		bool duplicate_found = false;
		int err;

		if (bt_addr_le_eq(&fp_bonds[i].nv.addr, BT_ADDR_LE_ANY)) {
			continue;
		}

		for (size_t j = i + 1; j < ARRAY_SIZE(fp_bonds); j++) {
			if (bt_addr_le_eq(&fp_bonds[j].nv.addr, &fp_bonds[i].nv.addr)) {
				/* Error during Fast Pair Procedure. */
				duplicate_found = true;
				LOG_DBG("Removing both duplicated bond entries at bootup");
				err = bond_delete(j);
				if (err) {
					LOG_ERR("Failed to delete bond (err %d)", err);
				}
			}
		}

		if (duplicate_found) {
			err = bond_remove(i);
			if (err) {
				LOG_ERR("Failed to remove bond (err %d)", err);
			}
		}
	}
}

static void validate_bonds(void)
{
	int err;

	/* Assert that zero-initialized addresses are equal to BT_ADDR_LE_ANY. */
	static const bt_addr_le_t addr;
	__ASSERT_NO_MSG(bt_addr_le_eq(&addr, BT_ADDR_LE_ANY));

	for (size_t identity = 0; identity < CONFIG_BT_ID_MAX; identity++) {
		bt_foreach_bond(identity, bonded_field_set, NULL);
	}

	handle_bond_duplicates();

	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		if (bt_addr_le_eq(&fp_bonds[i].nv.addr, BT_ADDR_LE_ANY)) {
			continue;
		}

		if (!fp_bonds[i].bonded) {
			LOG_DBG("Deleting not bonded entry");
			err = bond_delete(i);
			if (err) {
				LOG_ERR("Failed to delete bond (err %d)", err);
			}

			continue;
		}

		if (fp_bonds[i].nv.account_key_metadata_id == 0) {
			/* Fast Pair Procedure unfinished. */
			err = bond_remove(i);
			if (err) {
				LOG_ERR("Failed to remove bond (err %d)", err);
			}
		}
	}

	bond_remove_old_id();
}

static int fp_settings_validate(void)
{
	uint8_t id;
	int first_zero_idx = -1;
	int err;

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

	err = validate_ak_order();
	if (err) {
		return err;
	}

	validate_bonds();

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

static int bond_find(size_t *bond_idx, const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		if (fp_bonds[i].conn == conn) {
			*bond_idx = i;
			return 0;
		}
	}

	return -ESRCH;
}

int fp_storage_ak_save(const struct fp_account_key *account_key, const struct bt_conn *conn)
{
	if (!is_enabled) {
		return -EACCES;
	}

	uint8_t id;
	uint8_t index;
	struct account_key_data data;
	char name[SETTINGS_AK_NAME_MAX_SIZE];
	int err;

	size_t bond_idx;
	struct fp_bond bond;
	char bond_name[SETTINGS_BOND_NAME_MAX_SIZE];
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

	err = ak_name_gen(name, index);
	if (err) {
		return err;
	}

	if (conn) {
		err = bond_find(&bond_idx, conn);
		if (err) {
			return err;
		}

		if (fp_bonds[bond_idx].bonded) {
			bond = fp_bonds[bond_idx];

			bond.nv.account_key_metadata_id = id;

			err = bond_name_gen(bond_name, bond_idx);
			if (err) {
				return err;
			}

			err = settings_save_one(bond_name, &bond.nv, sizeof(bond.nv));
			if (err) {
				return err;
			}

			fp_bonds[bond_idx] = bond;
		} else {
			__ASSERT(false, "Account Key cannot be written by unbonded connection.");
		}
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

	if (conn && fp_bonds[bond_idx].bonded) {
		/* Procedure finished successfully. Setting conn to NULL. */
		fp_bonds[bond_idx].conn = NULL;
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

	if (ak_overwritten) {
		/* Account Key overwritten. Remove bonds releted with overwritten Account Key. */
		bond_remove_old_id();
	}

	return 0;
}

static int ak_metadata_id_get(uint8_t *id, const struct fp_account_key *account_key)
{
	for (size_t i = 0; i < account_key_count; i++) {
		if (!memcmp(account_key_list[i].key, account_key->key, FP_ACCOUNT_KEY_LEN)) {
			*id = ACCOUNT_KEY_METADATA_FIELD_GET(account_key_metadata[i], ID);
			return 0;
		}
	}

	return -ESRCH;
}

static int bond_free_idx_get(size_t *bond_idx)
{
	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		if (bt_addr_le_eq(&fp_bonds[i].nv.addr, BT_ADDR_LE_ANY)) {
			*bond_idx = i;
			return 0;
		}
	}

	return -ENOMEM;
}

int fp_storage_ak_bond_save(const struct bt_conn *conn, const struct fp_account_key *account_key)
{
	if (!is_enabled) {
		return -EACCES;
	}

	int err;
	uint8_t id;
	size_t bond_idx;
	struct fp_bond bond;
	char name[SETTINGS_BOND_NAME_MAX_SIZE];
	struct bt_conn_info conn_info;

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		LOG_ERR("Failed to get local conn info (err %d)", err);
		return err;
	}

	if (account_key) {
		err = ak_metadata_id_get(&id, account_key);
		if (err) {
			return err;
		}
	} else {
		/* The actual ID will be determined during Account Key write. */
		id = 0;
	}

	err = bond_free_idx_get(&bond_idx);
	if (err) {
		return err;
	}

	bond.conn = conn;
	bond.bonded = false;
	bond.nv.account_key_metadata_id = id;
	bond.nv.identity = conn_info.id;
	bond.nv.addr = *conn_info.le.dst;

	err = bond_name_gen(name, bond_idx);
	if (err) {
		return err;
	}

	err = settings_save_one(name, &bond.nv, sizeof(bond.nv));
	if (err) {
		return err;
	}

	fp_bonds[bond_idx] = bond;

	return 0;
}

static int bonded_duplicate_find(size_t *bond_idx, const struct bt_conn *conn)
{
	struct bt_conn_info conn_info;
	int err;

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		LOG_ERR("Failed to get local conn info (err %d)", err);
		return err;
	}

	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		if ((fp_bonds[i].conn != conn) &&
		    fp_bonds[i].bonded &&
		    !bt_addr_le_cmp(&fp_bonds[i].nv.addr, conn_info.le.dst)) {
			*bond_idx = i;
			return 0;
		}
	}

	return -ESRCH;
}

void fp_storage_ak_bonding_complete(const struct bt_conn *conn)
{
	if (!is_enabled) {
		return;
	}

	size_t bond_idx;
	size_t bond_duplicate_idx;
	int duplicate_find_err = 0;
	int err;

	err = bond_find(&bond_idx, conn);
	if (err) {
		return;
	}

	fp_bonds[bond_idx].bonded = true;

	while (!duplicate_find_err) {
		duplicate_find_err = bonded_duplicate_find(&bond_duplicate_idx, conn);
		if (!duplicate_find_err) {
			err = bond_delete(bond_duplicate_idx);
			if (err) {
				LOG_ERR("Failed to delete bond (err %d)", err);
				break;
			} else {
				LOG_DBG("Deleted duplicated bond");
			}
		}
	}

	if (fp_bonds[bond_idx].nv.account_key_metadata_id) {
		/* Account Key already associated (subsequent pairing procedure).
		 * Procedure finished successfully. Setting conn to NULL.
		 */
		fp_bonds[bond_idx].conn = NULL;
	}
}

static void bond_update_addr(struct bt_conn *conn, const bt_addr_le_t *new_addr)
{
	int err;
	size_t bond_idx;
	struct fp_bond bond;
	char name[SETTINGS_BOND_NAME_MAX_SIZE];

	err = bond_find(&bond_idx, conn);
	if (err) {
		return;
	}

	bond = fp_bonds[bond_idx];

	bond.nv.addr = *new_addr;

	err = bond_name_gen(name, bond_idx);
	if (err) {
		LOG_ERR("Failed to generate bond name (err %d)", err);
		return;
	}

	err = settings_save_one(name, &bond.nv, sizeof(bond.nv));
	if (err) {
		LOG_ERR("Failed to save bond (err %d)", err);
		return;
	}

	fp_bonds[bond_idx] = bond;
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	if (!is_enabled) {
		return;
	}

	LOG_DBG("Identity resolved");

	bond_update_addr(conn, identity);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (!is_enabled) {
		return;
	}

	size_t bond_idx;
	int err;

	err = bond_find(&bond_idx, conn);
	if (err) {
		return;
	}

	/* If at this point the conn is not set to NULL, the procedure is incomplete. */
	err = bond_remove(bond_idx);
	if (err) {
		LOG_ERR("Failed to remove bond (err %d)", err);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.disconnected = disconnected,
	.identity_resolved = identity_resolved,
};

static int bonded_by_addr_find(size_t *bond_idx, const bt_addr_le_t *addr)
{
	for (size_t i = 0; i < ARRAY_SIZE(fp_bonds); i++) {
		if (fp_bonds[i].bonded &&
		    !bt_addr_le_cmp(&fp_bonds[i].nv.addr, addr)) {
			*bond_idx = i;
			return 0;
		}
	}

	return -ESRCH;
}

static void bond_deleted(uint8_t id, const bt_addr_le_t *peer)
{
	size_t bond_idx;
	int err;

	LOG_DBG("Bluetooth bond deleted");
	err = bonded_by_addr_find(&bond_idx, peer);
	if (err) {
		return;
	}

	err = bond_delete(bond_idx);
	if (err) {
		LOG_ERR("Failed to delete bond (err %d)", err);
	}
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.bond_deleted = bond_deleted
};

void fp_storage_ak_ram_clear(void)
{
	memset(account_key_list, 0, sizeof(account_key_list));
	memset(account_key_metadata, 0, sizeof(account_key_metadata));
	account_key_count = 0;

	memset(account_key_order, 0, sizeof(account_key_order));

	memset(fp_bonds, 0, sizeof(fp_bonds));

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

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		return err;
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

	int err;

	is_enabled = false;

	err = bt_conn_auth_info_cb_unregister(&conn_auth_info_callbacks);
	if (err) {
		return err;
	}

	return 0;
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
	bool was_enabled = is_enabled;

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

	for (uint8_t index = 0; index < ARRAY_SIZE(fp_bonds); index++) {
		if (bt_addr_le_eq(&fp_bonds[index].nv.addr, BT_ADDR_LE_ANY)) {
			continue;
		}

		err = bt_unpair(fp_bonds[index].nv.identity, &fp_bonds[index].nv.addr);
		if (err) {
			LOG_WRN("Failed to unpair (err %d). This might happen in some edge cases "
				"when the Fast Pair storage subsystem is trying to unpair a bond "
				"that has already been unpaired.", err);
		}

		err = fp_settings_bond_delete(index);
		if (err) {
			return err;
		}
	}

	err = settings_delete(SETTINGS_AK_ORDER_FULL_NAME);
	if (err) {
		return err;
	}

	fp_storage_ak_ram_clear();
	if (was_enabled) {
		err = fp_storage_ak_init();
		if (err) {
			return err;
		}
	}

	return 0;
}

static void reset_prepare(void)
{
	/* intentionally left empty */
}

SETTINGS_STATIC_HANDLER_DEFINE(fp_storage_ak, SETTINGS_AK_SUBTREE_NAME, NULL, fp_settings_set,
			       NULL, NULL);

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_storage_ak, fp_storage_ak_reset, reset_prepare,
				   fp_storage_ak_init, fp_storage_ak_uninit);
