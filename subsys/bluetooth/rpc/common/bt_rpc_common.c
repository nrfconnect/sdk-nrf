/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <nrf_rpc/nrf_rpc_ipc.h>
#include <nrf_rpc_cbor.h>

#include "bt_rpc_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(BT_RPC, CONFIG_BT_RPC_LOG_LEVEL);

BUILD_ASSERT(!IS_ENABLED(CONFIG_BT_BREDR), "BT_RPC does not support BR/EDR");

NRF_RPC_IPC_TRANSPORT(bt_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "bt_rpc_ept");
NRF_RPC_GROUP_DEFINE(bt_rpc_grp, "bt_rpc", &bt_rpc_tr, NULL, NULL, NULL);

#if CONFIG_BT_RPC_INITIALIZE_NRF_RPC
static void err_handler(const struct nrf_rpc_err_report *report)
{
	LOG_ERR("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
		report->code);
	k_oops();
}

static int serialization_init(void)
{

	int err;

	LOG_DBG("Init begin");

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -EINVAL;
	}

	LOG_DBG("Init done\n");

	return 0;
}

SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif /* CONFIG_BT_RPC_INITIALIZE_NRF_RPC */

enum {
	CHECK_ENTRY_FLAGS,
	CHECK_ENTRY_UINT,
	CHECK_ENTRY_STR,
};

struct check_list_entry {
	uint8_t type;
	uint8_t size;
	union {
		uint32_t value;
		const char *str_value;
	};
	const char *name;
};

#define DEF_
#define _GET_DEF2(a, b, c, ...) c
#define _GET_DEF1(...) _GET_DEF2(__VA_ARGS__)
#define _GET_DEF(x) (_GET_DEF1(DEF_##x, DEF_##x, x))

#define _GET_DEF_STR(_value) #_value

#if defined(CONFIG_BT_RPC_HOST)

#define CHECK_LIST_ENTRY_TYPE uint8_t
#define STR_CHECK_LIST_ENTRY_TYPE char

#define CHECK_FLAGS(a, b, c, d, e, f, g, h) ( \
	(IS_ENABLED(a) ? BIT(0) : 0) | \
	(IS_ENABLED(b) ? BIT(1) : 0) | \
	(IS_ENABLED(c) ? BIT(2) : 0) | \
	(IS_ENABLED(d) ? BIT(3) : 0) | \
	(IS_ENABLED(e) ? BIT(4) : 0) | \
	(IS_ENABLED(f) ? BIT(5) : 0) | \
	(IS_ENABLED(g) ? BIT(6) : 0) | \
	(IS_ENABLED(h) ? BIT(7) : 0))

#define CHECK_UINT8(_value) _GET_DEF(_value)

#define CHECK_UINT16(_value) _GET_DEF(_value) & 0xFF, \
	(_GET_DEF(_value) >> 8) & 0xFF

#define CHECK_UINT8_PAIR(_value1, _value2) _GET_DEF(_value2), _GET_DEF(_value1)

#define CHECK_UINT16_PAIR(_value1, _value2) _GET_DEF(_value2) & 0xFF, \
	(_GET_DEF(_value2) >> 8) & 0xFF, \
	_GET_DEF(_value1) & 0xFF, \
	(_GET_DEF(_value1) >> 8) & 0xFF

#define CHECK_STR_BEGIN()
#define CHECK_STR(_value) _GET_DEF_STR(_value)
#define CHECK_STR_END()

#else

#define CHECK_LIST_ENTRY_TYPE struct check_list_entry
#define STR_CHECK_LIST_ENTRY_TYPE struct check_list_entry

#define CHECK_FLAGS(a, b, c, d, e, f, g, h) \
	{ \
		.type = CHECK_ENTRY_FLAGS, \
		.size = 1, \
		.value = (IS_ENABLED(a) ? BIT(0) : 0) | \
			 (IS_ENABLED(b) ? BIT(1) : 0) | \
			 (IS_ENABLED(c) ? BIT(2) : 0) | \
			 (IS_ENABLED(d) ? BIT(3) : 0) | \
			 (IS_ENABLED(e) ? BIT(4) : 0) | \
			 (IS_ENABLED(f) ? BIT(5) : 0) | \
			 (IS_ENABLED(g) ? BIT(6) : 0) | \
			 (IS_ENABLED(h) ? BIT(7) : 0), \
		.name = #a "\0" #b "\0" #c "\0" #d "\0" \
			#e "\0" #f "\0" #g "\0" #h "\0" \
	}

#define CHECK_UINT8(_value)  \
	{ \
		.type = CHECK_ENTRY_UINT, \
		.size = sizeof(uint8_t), \
		.value = _GET_DEF(_value), \
		.name = #_value, \
	}

#define CHECK_UINT16(_value)  \
	{ \
		.type = CHECK_ENTRY_UINT, \
		.size = sizeof(uint16_t), \
		.value = _GET_DEF(_value), \
		.name = #_value, \
	}

#define CHECK_UINT8_PAIR(_value1, _value2)  \
	{ \
		.type = CHECK_ENTRY_UINT, \
		.size = sizeof(uint8_t), \
		.value = _GET_DEF(_value1), \
		.name = #_value1 " (net: " #_value2 ")", \
	}, \
	{ \
		.type = CHECK_ENTRY_UINT, \
		.size = sizeof(uint8_t), \
		.value = _GET_DEF(_value2), \
		.name = #_value2 " (net: " #_value1 ")", \
	}

#define CHECK_UINT16_PAIR(_value1, _value2) \
	{ \
		.type = CHECK_ENTRY_UINT, \
		.size = sizeof(uint16_t), \
		.value = _GET_DEF(_value1), \
		.name = #_value1 " (net: " #_value2 ")", \
	}, \
	{ \
		.type = CHECK_ENTRY_UINT, \
		.size = sizeof(uint16_t), \
		.value = _GET_DEF(_value2), \
		.name = #_value2 " (net: " #_value1 ")", \
	}

#define CHECK_STR_BEGIN() {

#define CHECK_STR(_value) \
	{ \
		.type = CHECK_ENTRY_STR, \
		.size = 0, \
		.str_value = _GET_DEF_STR(_value), \
		.name = #_value, \
	}, \

#define CHECK_STR_END() }

#endif

/*
 * Default values that will be put into the check list if specific
 * configuration is not defined. Each define MUST end with a comma.
 * Mostly, it is the maximum integer value of a specific size.
 */
#define DEF_CONFIG_BT_MAX_CONN 0xFF,
#define DEF_CONFIG_BT_ID_MAX 0xFF,
#define DEF_CONFIG_BT_EXT_ADV_MAX_ADV_SET 0xFF,
#define DEF_CONFIG_BT_DEVICE_NAME_MAX 0xFF,
#define DEF_CONFIG_BT_PER_ADV_SYNC_MAX 0xFF,

static const CHECK_LIST_ENTRY_TYPE check_table[] = {
	CHECK_FLAGS(
		1,
		CONFIG_BT_CENTRAL,
		CONFIG_BT_PERIPHERAL,
		CONFIG_BT_FILTER_ACCEPT_LIST,
		CONFIG_BT_USER_PHY_UPDATE,
		CONFIG_BT_USER_DATA_LEN_UPDATE,
		CONFIG_BT_PRIVACY,
		CONFIG_BT_SCAN_WITH_IDENTITY),
	CHECK_FLAGS(
		CONFIG_BT_REMOTE_VERSION,
		CONFIG_BT_SMP,
		CONFIG_BT_CONN,
		CONFIG_BT_REMOTE_INFO,
		CONFIG_BT_FIXED_PASSKEY,
		CONFIG_BT_SMP_APP_PAIRING_ACCEPT,
		CONFIG_BT_EXT_ADV,
		CONFIG_BT_OBSERVER),
	CHECK_FLAGS(
		CONFIG_BT_ECC,
		CONFIG_BT_DEVICE_NAME_DYNAMIC,
		CONFIG_BT_SMP_SC_PAIR_ONLY,
		CONFIG_BT_PER_ADV,
		CONFIG_BT_PER_ADV_SYNC,
		CONFIG_BT_MAX_PAIRED,
		CONFIG_BT_SETTINGS_CCC_LAZY_LOADING,
		CONFIG_BT_BROADCASTER),
	CHECK_FLAGS(
		CONFIG_BT_SETTINGS,
		CONFIG_BT_GATT_CLIENT,
		CONFIG_BT_RPC_INTERNAL_FUNCTIONS,
		CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC,
		0,
		0,
		0,
		0),
	CHECK_UINT8(CONFIG_BT_MAX_CONN),
	CHECK_UINT8(CONFIG_BT_ID_MAX),
	CHECK_UINT8(CONFIG_BT_EXT_ADV_MAX_ADV_SET),
	CHECK_UINT8(CONFIG_BT_DEVICE_NAME_MAX),
	CHECK_UINT8(CONFIG_BT_PER_ADV_SYNC_MAX),
	CHECK_UINT16(CONFIG_BT_DEVICE_APPEARANCE),
	CHECK_UINT16_PAIR(CONFIG_CBKPROXY_OUT_SLOTS, CONFIG_CBKPROXY_IN_SLOTS),
};

static const STR_CHECK_LIST_ENTRY_TYPE str_check_list[] =
	CHECK_STR_BEGIN()
	CHECK_STR(CONFIG_BT_DEVICE_NAME)
	CHECK_STR_END();

#if defined(CONFIG_BT_RPC_HOST)
void bt_rpc_get_check_list(uint8_t *data, size_t size)
{
	size_t str_copy_bytes = sizeof(str_check_list);

	memset(data, 0, size);

	if (size < sizeof(check_table)) {
		return;
	} else if (size < sizeof(check_table) + str_copy_bytes) {
		str_copy_bytes = size - sizeof(check_table);
	}

	memcpy(data, check_table, sizeof(check_table));
	memcpy(&data[sizeof(check_table)], str_check_list, str_copy_bytes);

	LOG_DBG("Check table size: %d+%d=%d (copied %d)", sizeof(check_table),
		sizeof(str_check_list),
		sizeof(check_table) + sizeof(str_check_list),
		sizeof(check_table) + str_copy_bytes);
}

#else
static bool validate_flags(const CHECK_LIST_ENTRY_TYPE *entry, uint8_t flags)
{
	size_t i;
	uint8_t host;
	uint8_t client;
	const char *name;

	if (entry->value != flags) {
		name = entry->name;

		for (i = 0; i < 8; i++) {
			host = ((flags >> i) & 1);
			client = ((entry->value >> i) & 1);
			if (host != client) {
				LOG_ERR("Missmatched %s: net=%d, app=%d", name, host, client);
			}
			name += strlen(name) + 1;
		}

		return false;
	} else {
		return true;
	}
}

static bool validate_uint(const CHECK_LIST_ENTRY_TYPE *entry, const uint8_t *data)
{
	uint32_t value = 0;
	size_t i;

	for (i = 0; i < entry->size; i++) {
		value <<= 8;
		value |= data[entry->size - i - 1];
	}

	if (value != entry->value) {
		LOG_ERR("Missmatched %s: net=%d, app=%d", entry->name, value, entry->value);
		return false;
	} else {
		return true;
	}
}

static bool validate_str(const CHECK_LIST_ENTRY_TYPE *entry, uint8_t **data, size_t *size)
{
	size_t client_len;
	size_t host_len;
	const char *client = entry->str_value;
	const char *host = (const char *) (*data);

	if (*size == 0) {
		LOG_ERR("Missmatched BT RPC config.");
		return false;
	}

	host_len = strlen(host) + 1;
	*data += host_len;
	*size -= host_len;
	client_len = strlen(client) + 1;

	if (client_len != host_len || memcmp(client, host, client_len) != 0) {
		if (host[0] != '"') {
			host = "#undef";
		}
		if (client[0] != '"') {
			client = "#undef";
		}
		LOG_ERR("Missmatched %s: net=%s, app=%s", entry->name, host, client);
		return false;
	}

	return true;
}

static bool check_table_part(uint8_t **data, size_t *size,
			     const CHECK_LIST_ENTRY_TYPE *table, size_t items)
{
	size_t i;
	const CHECK_LIST_ENTRY_TYPE *entry;
	bool ok = true;

	for (i = 0; i < items; i++) {
		entry = &table[i];
		if (*size < entry->size) {
			LOG_ERR("Missmatched BT RPC config.");
			return false;
		}
		switch (entry->type) {
		case CHECK_ENTRY_FLAGS:
			ok = validate_flags(entry, (*data)[0]) && ok;
			break;

		case CHECK_ENTRY_UINT:
			ok = validate_uint(entry, (*data)) && ok;
			break;

		case CHECK_ENTRY_STR:
			ok = validate_str(entry, data, size) && ok;
			break;
		}
		*data += entry->size;
		*size -= entry->size;
	}

	return ok;
}

bool bt_rpc_validate_check_list(uint8_t *data, size_t size)
{
	bool ok = true;

	if (data[0] == 0) {
		LOG_ERR("Missmatched BT RPC config.");
		return false;
	}

	data[size - 1] = 0;

	ok = check_table_part(&data, &size, check_table, ARRAY_SIZE(check_table));
	ok = check_table_part(&data, &size, str_check_list, ARRAY_SIZE(str_check_list)) && ok;

	if (size != 1) {
		LOG_ERR("Missmatched BT RPC config.");
		return false;
	}

	if (ok) {
		LOG_INF("Matching configuration");
	}

	return ok;
}

size_t bt_rpc_calc_check_list_size(void)
{
	size_t i;
	size_t size = 0;
	size_t str_size = 0;

	for (i = 0; i < ARRAY_SIZE(check_table); i++) {
		size += check_table[i].size;
	}

	for (i = 0; i < ARRAY_SIZE(str_check_list); i++) {
		str_size += strlen(str_check_list[i].str_value) + 1;
	}
	str_size++;

	LOG_DBG("Check table size: %d+%d=%d", size, str_size, size + str_size);

	return size + str_size;
}
#endif

int bt_rpc_pool_reserve(atomic_t *pool_mask)
{
	int number;
	atomic_val_t mask_shadow;
	atomic_val_t this_mask;

	mask_shadow = atomic_get(pool_mask);
	do {
		number = __CLZ(mask_shadow);
		if (number >= 32) {
			return -1;
		}
		this_mask = (0x80000000u >> number);
		mask_shadow = atomic_and(pool_mask, ~this_mask);
	} while ((mask_shadow & this_mask) == 0);

	return number;
}

void bt_rpc_pool_release(atomic_t *pool_mask, int number)
{
	if (number >= 0) {
		atomic_or(pool_mask, 0x80000000u >> number);
	}
}
