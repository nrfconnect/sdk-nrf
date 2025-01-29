/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/mesh.h>
#include <mesh/crypto.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(key_importer, CONFIG_BT_MESH_KEYS_LOG_LEVEL);

struct net_key_legacy {
	uint8_t unused:1,
		kr_phase:7;
	uint8_t val[2][16];
} __packed;

struct net_key_val {
	uint8_t unused:1,
		kr_phase:7;
	struct bt_mesh_key val[2];
} __packed;

struct app_key_legacy {
	uint16_t net_idx;
	bool updated;
	uint8_t val[2][16];
} __packed;

struct app_key_val {
	uint16_t net_idx;
	bool updated;
	struct bt_mesh_key val[2];
} __packed;

struct net_legacy {
	uint16_t primary_addr;
	uint8_t dev_key[16];
} __packed;

struct net_val {
	uint16_t primary_addr;
	struct bt_mesh_key dev_key;
} __packed;

struct node_val_legacy {
	uint16_t net_idx;
	uint8_t num_elem;
	uint8_t flags;
	uint8_t uuid[16];
	uint8_t dev_key[16];
} __packed;

struct node_val {
	uint16_t net_idx;
	uint8_t num_elem;
	uint8_t flags;
	uint8_t uuid[16];
	struct bt_mesh_key dev_key;
} __packed;

static int import_net_keys(const char *path, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct net_key_legacy legacy_net_key;
	struct net_key_val net_key;
	struct bt_mesh_key key_id;
	uint8_t kr_phase;
	int err;

	if (len != sizeof(struct net_key_legacy)) {
		LOG_DBG("Network key %s is not legacy", path);
		return 0;
	}

	err = read_cb(cb_arg, &legacy_net_key, len);
	if (err <= 0) {
		LOG_ERR("Failed to load legacy network key %s (err %d)", path, err);
		return err == 0 ? -EIO : err;
	}

	kr_phase = legacy_net_key.kr_phase;

	LOG_DBG("kr phase %d", kr_phase);
	LOG_HEXDUMP_DBG(&legacy_net_key.val[0][0], 16, "key one:");

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_NET, &legacy_net_key.val[0][0], &key_id);
	if (err) {
		LOG_ERR("Failed to import first network key %s (err %d)", path, err);
		return err;
	}

	net_key.val[0].key = key_id.key;

	net_key.kr_phase = kr_phase;
	net_key.unused = 0U;
	if (kr_phase == BT_MESH_KR_NORMAL) {
		net_key.val[1].key = PSA_KEY_ID_NULL;
		goto store;
	}

	LOG_HEXDUMP_DBG(&legacy_net_key.val[1][0], 16, "key two:");

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_NET, &legacy_net_key.val[1][0], &key_id);
	if (err) {
		LOG_ERR("Failed to import second network key %s (err %d)", path, err);
		return err;
	}

	net_key.val[1].key = key_id.key;

store:

	err = settings_save_one(path, &net_key, sizeof(net_key));
	if (err) {
		LOG_ERR("Failed to store network key: %s (err %d)", path, err);
		return err;
	}

	LOG_INF("%s successfully imported", path);

	return 0;
}

static int import_app_keys(const char *path, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct app_key_legacy legacy_app_key;
	struct app_key_val app_key;
	struct bt_mesh_key key_id;
	bool updated;
	int err;

	if (len != sizeof(struct app_key_legacy)) {
		LOG_DBG("Application key %s is not legacy", path);
		return 0;
	}

	err = read_cb(cb_arg, &legacy_app_key, len);
	if (err <= 0) {
		LOG_ERR("Failed to load application key %s (err %d)", path, err);
		return err == 0 ? -EIO : err;
	}

	updated = legacy_app_key.updated;

	LOG_HEXDUMP_DBG(&legacy_app_key.val[0][0], 16, "key one:");

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_APP, &legacy_app_key.val[0][0], &key_id);
	if (err) {
		LOG_ERR("Failed to import first application key %s (err %d)", path, err);
		return err;
	}

	app_key.val[0].key = key_id.key;
	app_key.net_idx = legacy_app_key.net_idx;
	app_key.updated = updated;
	if (!updated) {
		app_key.val[1].key = PSA_KEY_ID_NULL;
		goto store;
	}

	LOG_HEXDUMP_DBG(&legacy_app_key.val[1][0], 16, "key two:");

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_APP, &legacy_app_key.val[1][0], &key_id);
	if (err) {
		LOG_ERR("Failed to import second application key %s (err %d)", path, err);
		return err;
	}

	app_key.val[1].key = key_id.key;

store:

	err = settings_save_one(path, &app_key, sizeof(app_key));
	if (err) {
		LOG_ERR("Failed to store application key: %s (err %d)", path, err);
		return err;
	}

	LOG_INF("%s successfully imported", path);

	return 0;
}

static int import_dev_keys(const char *path, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct net_legacy legacy_net;
	struct net_val net;
	struct bt_mesh_key key_id;
	int err;

	if (len != sizeof(struct net_legacy)) {
		LOG_DBG("Network is not legacy");
		return 0;
	}

	err = read_cb(cb_arg, &legacy_net, len);
	if (err <= 0) {
		LOG_ERR("Failed to load legacy network %s (err %d)", path, err);
		return err == 0 ? -EIO : err;
	}

	LOG_HEXDUMP_DBG(&legacy_net.dev_key, 16, "device key:");

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_DEV, legacy_net.dev_key, &key_id);
	if (err) {
		LOG_ERR("Failed to import device key %s (err %d)", path, err);
		return err;
	}

	net.dev_key.key = key_id.key;
	net.primary_addr = legacy_net.primary_addr;

	err = settings_save_one(path, &net, sizeof(net));
	if (err) {
		LOG_ERR("Failed to store network: %s (err %d)", path, err);
		return err;
	}

	LOG_INF("%s successfully imported", path);

	return 0;
}

static int import_devcand_keys(const char *path, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	uint8_t legacy_key[16];
	struct bt_mesh_key key;
	int err;

	if (len != sizeof(legacy_key)) {
		LOG_DBG("Device key candidate is not legacy");
		return 0;
	}

	err = read_cb(cb_arg, &legacy_key, len);
	if (err <= 0) {
		LOG_ERR("Failed to load device key candidate (err %d)", err);
		return err == 0 ? -EIO : err;
	}

	LOG_HEXDUMP_DBG(legacy_key, 16, "device key: ");

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_DEV, legacy_key, &key);
	if (err) {
		LOG_ERR("Failed to import device key candidate (err %d)", err);
		return err;
	}

	err = settings_save_one(path, &key, sizeof(key));
	if (err) {
		LOG_ERR("Failed to store device key: %s (err %d)", path, err);
		return err;
	}

	LOG_INF("%s successfully imported", path);

	return 0;
}

static int import_cdb_dev_keys(const char *path, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct node_val_legacy legacy_node;
	struct node_val node;
	struct bt_mesh_key key_id;
	int err;

	if (len != sizeof(struct node_val_legacy)) {
		LOG_DBG("cdb node's device key is not legacy");
		return 0;
	}

	err = read_cb(cb_arg, &legacy_node, len);
	if (err <= 0) {
		LOG_ERR("Failed to load cdb legacy node (err %d)", err);
		return err == 0 ? -EIO : err;
	}

	LOG_HEXDUMP_DBG(&legacy_node.dev_key, 16, "device key:");

	err = bt_mesh_key_import(BT_MESH_KEY_TYPE_DEV, legacy_node.dev_key, &key_id);
	if (err) {
		LOG_ERR("Failed to import cdb device key (err %d)", err);
		return err;
	}

	node.dev_key.key = key_id.key;
	node.net_idx = legacy_node.net_idx;
	node.num_elem = legacy_node.num_elem;
	node.flags = legacy_node.flags;
	memcpy(node.uuid, legacy_node.uuid, 16);

	err = settings_save_one(path, &node, sizeof(node));
	if (err) {
		LOG_ERR("Failed to store cdb node: %s (err %d)", path, err);
		return err;
	}

	LOG_INF("%s successfully imported", path);

	return 0;
}

static int get_idx(const char *name)
{
	const char *next;

	settings_name_next(name, &next);

	if (!next) {
		return -EINVAL;
	}

	return strtol(next, NULL, 16);
}

static int load_keys_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg,
			void *param)
{
	const char *next;
	char path[28];
	int idx;

	if (!name) {
		LOG_ERR("Insufficient number of arguments");
		return -ENOENT;
	}

	LOG_DBG("Load name: %s", name);

	if (!strncmp(name, "NetKey", strlen("NetKey"))) {
		idx = get_idx(name);

		if (idx < 0) {
			goto not_found;
		}

		snprintk(path, sizeof(path), "bt/mesh/NetKey/%x", idx);
		return import_net_keys(path, len, read_cb, cb_arg);
	}

	if (!strncmp(name, "AppKey", strlen("AppKey"))) {
		idx = get_idx(name);

		if (idx < 0) {
			goto not_found;
		}
		snprintk(path, sizeof(path), "bt/mesh/AppKey/%x", idx);
		return import_app_keys(path, len, read_cb, cb_arg);
	}

	if (!strncmp(name, "Net", strlen("Net"))) {
		snprintk(path, sizeof(path), "bt/mesh/Net");
		return import_dev_keys(path, len, read_cb, cb_arg);
	}

	if (!strncmp(name, "DevKeyC", strlen("DevKeyC"))) {
		snprintk(path, sizeof(path), "bt/mesh/DevKeyC");
		return import_devcand_keys(path, len, read_cb, cb_arg);
	}

	if (strncmp(name, "cdb", strlen("cdb"))) {
		return 0;
	}

	settings_name_next(name, &next);

	if (!next) {
		goto not_found;
	}

	if (strncmp(next, "Net", strlen("Net")) == 0) {
		return 0;
	}

	idx = get_idx(next);

	if (idx < 0) {
		goto not_found;
	}

	if (!strncmp(next, "Node", strlen("Node"))) {
		snprintk(path, sizeof(path), "bt/mesh/cdb/Node/%x", idx);
		return import_cdb_dev_keys(path, len, read_cb, cb_arg);
	}

	if (!strncmp(next, "Subnet", strlen("Subnet"))) {
		snprintk(path, sizeof(path), "bt/mesh/cdb/Subnet/%x", idx);
		return import_net_keys(path, len, read_cb, cb_arg);
	}

	if (!strncmp(next, "AppKey", strlen("AppKey"))) {
		snprintk(path, sizeof(path), "bt/mesh/cdb/AppKey/%x", idx);
		return import_app_keys(path, len, read_cb, cb_arg);
	}

	return 0;

not_found:
	LOG_ERR("Unknown key %s", name);
	return -ENOENT;
}

static int import_keys(void)
{
	int err;

	err = settings_subsys_init();
	if (err) {
		LOG_ERR("Settings initialization failed (err %d)", err);
		return err;
	}

	err = bt_mesh_crypto_init();
	if (err) {
		LOG_ERR("Crypto librarry initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("Importing keys...");
	settings_load_subtree_direct("bt/mesh", load_keys_cb, NULL);
	LOG_INF("Importing keys done");

	return 0;
}

SYS_INIT(import_keys, APPLICATION, CONFIG_BT_MESH_KEY_IMPORTER_PRIO);
