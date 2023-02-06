/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <string.h>
#include <zephyr/settings/settings.h>
#include <zephyr/device.h>

#include "storage_mock.h"

struct settings_data {
	sys_snode_t node;
	char *name;
	char *val;
	size_t val_len;
};

static sys_slist_t settings_list;


void storage_mock_clear(void)
{
	while (!sys_slist_is_empty(&settings_list)) {
		sys_snode_t *cur_node = sys_slist_get(&settings_list);
		struct settings_data *data = CONTAINER_OF(cur_node, struct settings_data, node);

		k_free(data->val);
		k_free(data->name);
		k_free(data);
	}
}

static ssize_t settings_mock_read_fn(void *back_end, void *data, size_t len)
{
	struct settings_data *settings_data = back_end;

	zassert_true(len <= settings_data->val_len, "Invalid readout length");
	memcpy(data, settings_data->val, len);

	return len;
}

static int settings_mock_load(struct settings_store *cs, const struct settings_load_arg *arg)
{
	zassert_is_null(arg->subtree, "Reading only specified subtree is not supported");

	int err = 0;
	sys_snode_t *cur_node;

	SYS_SLIST_FOR_EACH_NODE(&settings_list, cur_node) {
		struct settings_data *data = CONTAINER_OF(cur_node, struct settings_data, node);

		err = settings_call_set_handler(data->name, data->val_len, settings_mock_read_fn,
						data, arg);

		if (err) {
			break;
		}
	}

	return err;
}

static int settings_mock_save(struct settings_store *cs, const char *name, const char *value,
			      size_t val_len)
{
	const static size_t max_name_len = 64;

	struct settings_data *record;
	bool found = false;
	size_t name_len = strnlen(name, max_name_len);

	zassert_not_equal(name_len, max_name_len, "Too long settings key");

	sys_snode_t *cur_node;

	/* Update record if exists. */
	SYS_SLIST_FOR_EACH_NODE(&settings_list, cur_node) {
		record = CONTAINER_OF(cur_node, struct settings_data, node);

		if (!strcmp(record->name, name)) {
			found = true;
			break;
		}
	}

	if (found) {
		if (val_len == 0) {
			bool ret;

			k_free(record->val);
			k_free(record->name);
			ret = sys_slist_find_and_remove(&settings_list, &(record->node));
			zassert_true(ret, "Unable to delete settings item");
			k_free(record);

			return 0;
		}

		if (val_len != record->val_len) {
			k_free(record->val);

			record->val = k_malloc(val_len);
			zassert_not_null(record->val, "Heap too small. Increase heap size.");
			record->val_len = val_len;
		}

		memcpy(record->val, value, val_len);

		return 0;
	}

	if (val_len == 0) {
		return 0;
	}

	record = k_malloc(sizeof(*record));
	zassert_not_null(record, "Heap too small. Increase heap size.");

	record->name = k_malloc(name_len + 1);
	zassert_not_null(record->name, "Heap too small. Increase heap size.");
	strcpy(record->name, name);

	record->val = k_malloc(val_len);
	zassert_not_null(record->val, "Heap too small. Increase heap size.");
	memcpy(record->val, value, val_len);
	record->val_len = val_len;

	sys_slist_append(&settings_list, &record->node);

	return 0;
}

static struct settings_store_itf settings_mock_itf = {
	.csi_load = settings_mock_load,
	.csi_save = settings_mock_save,
};

static struct settings_store settings_mock_store = {
	.cs_itf = &settings_mock_itf
};

int settings_mock_init(const struct device *unused)
{
	ARG_UNUSED(unused);
	sys_slist_init(&settings_list);

	settings_dst_register(&settings_mock_store);
	settings_src_register(&settings_mock_store);

	return 0;
}

SYS_INIT(settings_mock_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
