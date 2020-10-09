/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <string.h>
#include <bluetooth/scan.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(nrf_bt_scan, CONFIG_BT_SCAN_LOG_LEVEL);

#define BT_SCAN_UUID_128_SIZE 16

#define MODE_CHECK (BT_SCAN_NAME_FILTER | BT_SCAN_ADDR_FILTER | \
	BT_SCAN_SHORT_NAME_FILTER | BT_SCAN_APPEARANCE_FILTER | \
	BT_SCAN_UUID_FILTER | BT_SCAN_MANUFACTURER_DATA_FILTER)

/* Scan filter mutex. */
K_MUTEX_DEFINE(scan_mutex);

/* Scanning control structure used to
 * compare matching filters, their mode and event generation.
 */
struct bt_scan_control {
	/* Number of active filters. */
	uint8_t filter_cnt;

	/* Number of matched filters. */
	uint8_t filter_match_cnt;

	/* Indicates whether at least one filter has been fitted. */
	bool filter_match;

	/* Indicates in which mode filters operate. */
	bool all_mode;

	/* Inform that device is connectable. */
	bool connectable;

	/* Data needed to establish connection and advertising information. */
	struct bt_scan_device_info device_info;

	/* Scan filter status. */
	struct bt_scan_filter_match filter_status;
};

/* Name filter structure.
 */
struct bt_scan_name_filter {
	/* Names that the main application will scan for,
	 * and that will be advertised by the peripherals.
	 */
	char target_name[CONFIG_BT_SCAN_NAME_CNT][CONFIG_BT_SCAN_NAME_MAX_LEN];

	/* Name filter counter. */
	uint8_t cnt;

	/* Flag to inform about enabling or disabling this filter.
	 */
	bool enabled;
};

/* Short names filter structure.
 */
struct bt_scan_short_name_filter {
	struct {
		/* Short names that the main application will scan for,
		 * and that will be advertised by the peripherals.
		 */
		char target_name[CONFIG_BT_SCAN_SHORT_NAME_MAX_LEN];

		/* Minimum length of the short name. */
		uint8_t min_len;
	} name[CONFIG_BT_SCAN_SHORT_NAME_CNT];

	/* Short name filter counter. */
	uint8_t cnt;

	/* Flag to inform about enabling or disabling this filter. */
	bool enabled;
};

/* BLE Addresses filter structure.
 */
struct bt_scan_addr_filter {
	/* Addresses advertised by the peripherals. */
	bt_addr_le_t target_addr[CONFIG_BT_SCAN_ADDRESS_CNT];

	/* Address filter counter. */
	uint8_t cnt;

	/* Flag to inform about enabling or disabling this filter. */
	bool enabled;
};

/* Structure for storing different types of UUIDs */
struct bt_scan_uuid {
	/* Pointer to the appropriate type of UUID. **/
	struct bt_uuid *uuid;
	union {
		/* 16-bit UUID. */
		struct bt_uuid_16 uuid_16;

		/* 32-bit UUID. */
		struct bt_uuid_32 uuid_32;

		/* 128-bit UUID. */
		struct bt_uuid_128 uuid_128;
	} uuid_data;
};

/* UUIDs filter structure.
 */
struct bt_scan_uuid_filter {
	/* UUIDs that the main application will scan for,
	 * and that will be advertised by the peripherals.
	 */
	struct bt_scan_uuid uuid[CONFIG_BT_SCAN_UUID_CNT];

	/* UUID filter counter. */
	uint8_t cnt;

	/* Flag to inform about enabling or disabling this filter. */
	bool enabled;
};

struct bt_scan_appearance_filter {
	/* Apperances that the main application will scan for,
	 * and that will be advertised by the peripherals.
	 */
	uint16_t appearance[CONFIG_BT_SCAN_APPEARANCE_CNT];

	/* Appearance filter counter. */
	uint8_t cnt;

	/* Flag to inform about enabling or disabling this filter. */
	bool enabled;
};

/* Manufacturer data filter structure.
 */
struct bt_scan_manufacturer_data_filter {
	struct {
		/* Manufacturer data that the main application will scan for,
		 * and that will be advertised by the peripherals.
		 */
		uint8_t data[CONFIG_BT_SCAN_MANUFACTURER_DATA_MAX_LEN];

		/* Length of the manufacturere data that the main application
		 * will scan for.
		 */
		uint8_t data_len;
	} manufacturer_data[CONFIG_BT_SCAN_MANUFACTURER_DATA_CNT];

	/* Name filter counter. */
	uint8_t cnt;

	/* Flag to inform about enabling or disabling this filter. */
	bool enabled;
};

/* Filters data.
 * This structure contains all filter data and the information
 * about enabling and disabling any type of filters.
 * Flag all_filter_mode informs about the filter mode.
 * If this flag is set, then all types of enabled filters
 * must be matched for the module to send a notification to
 * the main application. Otherwise, it is enough to
 * match one of filters to send notification.
 */
struct bt_scan_filters {
	/* Name filter data. */
	struct bt_scan_name_filter name;

	/* Short name filter data. */
	struct bt_scan_short_name_filter short_name;

	/* Address filter data. */
	struct bt_scan_addr_filter addr;

	/* UUID filter data. */
	struct bt_scan_uuid_filter uuid;

	/* Appearance filter data. */
	struct bt_scan_appearance_filter appearance;

	/* Manufacturer data filter data. */
	struct bt_scan_manufacturer_data_filter manufacturer_data;

	/* Filter mode. If true, all set filters must be
	 * matched to generate an event.
	 */
	bool all_mode;
};

#if CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER
/* Connection attempts filter device */
struct conn_attempts_device {
	/* Filtered device address. */
	bt_addr_le_t addr;

	/* Number of the connection attempts. */
	size_t attempts;
};

/* Connection attempts filter. */
struct conn_attempts_filter {
	/* Array of the filtered devices. */
	struct conn_attempts_device device[CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER_LEN];

	/* The oldest device index. */
	uint32_t oldest_idx;

	/* Count of the filtered devices. */
	size_t count;
};
#endif /* CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER */

#if CONFIG_BT_SCAN_BLOCKLIST
/* Connection blocklist */
struct conn_blocklist {
	/* Array of the blocklist devices. */
	bt_addr_le_t addr[CONFIG_BT_SCAN_BLOCKLIST_LEN];

	/* Blocklist device count. */
	uint32_t count;
};
#endif /* CONFIG_BT_SCAN_BLOCKLIST */

/* Scanning module instance. Options for the different scanning modes.
 * This structure stores all module settings. It is used to enable
 * or disable scanning modes and to configure filters.
 */
static struct bt_scan {
	/* Filter data. */
	struct bt_scan_filters scan_filters;

	/* If set to true, the module automatically connects
	 * after a filter match.
	 */
	bool connect_if_match;

	/* Scan parameters required to initialize the module.
	 * Can be initialized as NULL. If NULL, the parameters required to
	 * initialize the module are loaded from the static configuration.
	 */
	struct bt_le_scan_param scan_param;

	/* Connection parameters. Can be initialized as NULL.
	 * If NULL, the default static configuration is used.
	 */
	struct bt_le_conn_param conn_param;

#if CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER
	/* Scan Connection attempts filter. */
	struct conn_attempts_filter attempts_filter;
#endif /* CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER */

#if CONFIG_BT_SCAN_BLOCKLIST
	/* Scan device blocklist. */
	struct conn_blocklist blocklist;
#endif /* CONFIG_BT_SCAN_BLOCKLIST */

} bt_scan;

static sys_slist_t callback_list;

void bt_scan_cb_register(struct bt_scan_cb *cb)
{
	if (!cb) {
		return;
	}

	sys_slist_append(&callback_list, &cb->node);
}

static void notify_filter_matched(struct bt_scan_device_info *device_info,
				  struct bt_scan_filter_match *filter_match,
				  bool connectable)
{
	struct bt_scan_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&callback_list, cb, node) {
		if (cb->cb_addr->filter_match) {
			cb->cb_addr->filter_match(device_info, filter_match,
						  connectable);
		}
	}
}

static void notify_filter_no_match(struct bt_scan_device_info *device_info,
				   bool connectable)
{
	struct bt_scan_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&callback_list, cb, node) {
		if (cb->cb_addr->filter_no_match) {
			cb->cb_addr->filter_no_match(device_info, connectable);
		}
	}
}

static void notify_connecting(struct bt_scan_device_info *device_info,
			      struct bt_conn *conn)
{
	struct bt_scan_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&callback_list, cb, node) {
		if (cb->cb_addr->connecting) {
			cb->cb_addr->connecting(device_info, conn);
		}
	}
}

static void notify_connecting_error(struct bt_scan_device_info *device_info)
{
	struct bt_scan_cb *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&callback_list, cb, node) {
		if (cb->cb_addr->connecting_error) {
			cb->cb_addr->connecting_error(device_info);
		}
	}
}

#if CONFIG_BT_SCAN_BLOCKLIST
static bool blocklist_device_check(const bt_addr_le_t *addr)
{
	bool blocklist_device = false;

	k_mutex_lock(&scan_mutex, K_FOREVER);

	for (size_t i = 0; i < bt_scan.blocklist.count; i++) {
		if (bt_addr_le_cmp(&bt_scan.blocklist.addr[i], addr) == 0) {
			blocklist_device = true;

			break;
		}
	}

	k_mutex_unlock(&scan_mutex);

	return blocklist_device;
}
#endif /* CONFIG_BT_SCAN_BLOCKLIST */

#if CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER
static void attempts_filter_force_add(struct conn_attempts_filter *filter,
				      const bt_addr_le_t *addr)
{
	/* Overwrite the oldest device */
	filter->device[filter->oldest_idx].attempts = 0;
	bt_addr_le_copy(&filter->device[filter->oldest_idx].addr, addr);

	if (filter->oldest_idx == (ARRAY_SIZE(filter->device) - 1)) {
		filter->oldest_idx = 0;

		return;
	}

	filter->oldest_idx++;
}

static void scan_attempts_filter_device_add(const bt_addr_le_t *addr)
{
	struct conn_attempts_filter *filter = &bt_scan.attempts_filter;
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	k_mutex_lock(&scan_mutex, K_FOREVER);

	/* Check if device is already in the filter array. */
	for (size_t i = 0; i < filter->count; i++) {
		struct conn_attempts_device *device = &filter->device[i];

		if (bt_addr_le_cmp(addr, &device->addr) == 0) {
			LOG_DBG("Device %s is already in the filter array",
				log_strdup(addr_str));
			goto out;
		}
	}

	if (filter->count >= ARRAY_SIZE(filter->device)) {
		LOG_DBG("Force adding %s device filter",
			log_strdup(addr_str));
		attempts_filter_force_add(filter, addr);
	} else {
		bt_addr_le_copy(&filter->device[filter->count].addr, addr);
		filter->count++;
	}

out:
	k_mutex_unlock(&scan_mutex);
}

static void device_conn_attempts_count(struct bt_conn *conn)
{
	const bt_addr_le_t *addr = bt_conn_get_dst(conn);
	struct conn_attempts_filter *filter = &bt_scan.attempts_filter;

	k_mutex_lock(&scan_mutex, K_FOREVER);

	for (size_t i = 0; i < filter->count; i++) {
		struct conn_attempts_device *device = &filter->device[i];

		if (bt_addr_le_cmp(addr, &device->addr) == 0) {
			if (device->attempts < CONFIG_BT_SCAN_CONN_ATTEMPTS_COUNT) {
				device->attempts++;
			}

			break;
		}
	}

	k_mutex_unlock(&scan_mutex);
}

static bool conn_attempts_exceeded(const bt_addr_le_t *addr)
{
	struct conn_attempts_filter *filter = &bt_scan.attempts_filter;
	char addr_str[BT_ADDR_LE_STR_LEN];
	bool attempts_exceeded = false;

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	k_mutex_lock(&scan_mutex, K_FOREVER);

	/* Check if the device is in the filter array. */
	for (size_t i = 0; i < filter->count; i++) {
		struct conn_attempts_device *device = &filter->device[i];

		if (bt_addr_le_cmp(addr, &device->addr) == 0) {
			if (device->attempts >= CONFIG_BT_SCAN_CONN_ATTEMPTS_COUNT) {
				LOG_DBG("Connection attempts count for %s exceeded",
					log_strdup(addr_str));
				attempts_exceeded = true;
			}

			break;
		}
	}

	k_mutex_unlock(&scan_mutex);

	return attempts_exceeded;
}

#endif /* CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER */

static bool scan_device_filter_check(const bt_addr_le_t *addr)
{
#if CONFIG_BT_SCAN_BLOCKLIST
	if (blocklist_device_check(addr)) {
		return false;
	}
#endif /* CONFIG_BT_SCAN_BLOCKLIST */

#if CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER
	if (conn_attempts_exceeded(addr)) {
		return false;
	}
#endif /* CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER */

	return true;
}

static void scan_connect_with_target(struct bt_scan_control *control,
				     const bt_addr_le_t *addr)
{
	int err;

	/* Return if the automatic connection is disabled. */
	if (!bt_scan.connect_if_match) {
		return;
	}

	/* Establish connection. */
	struct bt_conn *conn;

	/* Stop scanning. */
	bt_scan_stop();

	err = bt_conn_le_create(addr,
			       BT_CONN_LE_CREATE_CONN,
			       &bt_scan.conn_param, &conn);

	LOG_DBG("Connecting (%d)", err);

	if (err) {
		/* If an error occurred, send an event to
		 * the all interested.
		 */
		notify_connecting_error(&control->device_info);
	} else {
		notify_connecting(&control->device_info, conn);
		bt_conn_unref(conn);
	}
}

static bool adv_addr_compare(const bt_addr_le_t *target_addr,
			     struct bt_scan_control *control)
{
	const bt_addr_le_t *addr =
			bt_scan.scan_filters.addr.target_addr;
	uint8_t counter = bt_scan.scan_filters.addr.cnt;

	for (size_t i = 0; i < counter; i++) {
		if (bt_addr_le_cmp(target_addr, &addr[i]) == 0) {
			control->filter_status.addr.addr = &addr[i];

			return true;
		}
	}

	return false;
}

static bool is_addr_filter_enabled(void)
{
	return bt_scan.scan_filters.addr.enabled;
}

static void check_addr(struct bt_scan_control *control,
		       const bt_addr_le_t *addr)
{
	if (is_addr_filter_enabled()) {
		if (adv_addr_compare(addr, control)) {
			control->filter_match_cnt++;

			/* Information about the filters matched. */
			control->filter_status.addr.match = true;
			control->filter_match = true;
		}
	}
}

static int scan_addr_filter_add(const bt_addr_le_t *target_addr)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t *addr_filter =
			bt_scan.scan_filters.addr.target_addr;
	uint8_t counter = bt_scan.scan_filters.addr.cnt;

	/* If no memory for filter. */
	if (counter >= CONFIG_BT_SCAN_ADDRESS_CNT) {
		return -ENOMEM;
	}

	/* Check for duplicated filter. */
	for (size_t i = 0; i < counter; i++) {
		if (bt_addr_le_cmp(target_addr, &addr_filter[i]) == 0) {
			return 0;
		}
	}

	/* Add target address to filter. */
	bt_addr_le_copy(&addr_filter[counter], target_addr);

	LOG_DBG("Filter set on address type %i",
		addr_filter[counter].type);

	bt_addr_le_to_str(target_addr, addr, sizeof(addr));

	LOG_DBG("Address: %s", addr);

	/* Increase the address filter counter. */
	bt_scan.scan_filters.addr.cnt++;

	return 0;
}

static bool adv_name_cmp(const uint8_t *data,
			 uint8_t data_len,
			 const char *target_name)
{
	return strncmp(target_name, data, data_len) == 0;
}

static bool adv_name_compare(const struct bt_data *data,
			     struct bt_scan_control *control)
{
	struct bt_scan_name_filter const *name_filter =
			&bt_scan.scan_filters.name;
	uint8_t counter = bt_scan.scan_filters.name.cnt;
	uint8_t data_len = data->data_len;

	/* Compare the name found with the name filter. */
	for (size_t i = 0; i < counter; i++) {
		if (adv_name_cmp(data->data,
				 data_len,
				 name_filter->target_name[i])) {

			control->filter_status.name.name =
				name_filter->target_name[i];
			control->filter_status.name.len = data_len;

			return true;
		}
	}

	return false;
}

static bool is_name_filter_enabled(void)
{
	return bt_scan.scan_filters.name.enabled;
}

static void name_check(struct bt_scan_control *control,
		       const struct bt_data *data)
{
	if (is_name_filter_enabled()) {
		if (adv_name_compare(data, control)) {
			control->filter_match_cnt++;

			/* Information about the filters matched. */
			control->filter_status.name.match = true;
			control->filter_match = true;
		}
	}
}

static int scan_name_filter_add(const char *name)
{
	uint8_t counter = bt_scan.scan_filters.name.cnt;
	size_t name_len;

	/* If no memory for filter. */
	if (counter >= CONFIG_BT_SCAN_NAME_CNT) {
		return -ENOMEM;
	}

	name_len = strlen(name);

	/* Check the name length. */
	if ((name_len == 0) || (name_len > CONFIG_BT_SCAN_NAME_MAX_LEN)) {
		return -EINVAL;
	}

	/* Check for duplicated filter. */
	for (size_t i = 0; i < counter; i++) {
		if (!strcmp(bt_scan.scan_filters.name.target_name[i], name)) {
			return 0;
		}
	}

	/* Add name to filter. */
	memcpy(bt_scan.scan_filters.name.target_name[counter],
	       name, name_len);

	bt_scan.scan_filters.name.cnt++;

	LOG_DBG("Adding filter on %s name", name);

	return 0;
}

static bool adv_short_name_cmp(const uint8_t *data,
			       uint8_t data_len,
			       const char *target_name,
			       uint8_t short_name_min_len)
{
	if ((data_len >= short_name_min_len) &&
	    (strncmp(target_name, data, data_len) == 0)) {
		return true;
	}

	return false;
}

static bool adv_short_name_compare(const struct bt_data *data,
				   struct bt_scan_control *control)
{
	const struct bt_scan_short_name_filter *name_filter =
			&bt_scan.scan_filters.short_name;
	uint8_t counter = bt_scan.scan_filters.short_name.cnt;
	uint8_t data_len = data->data_len;

	/* Compare the name found with the name filters. */
	for (size_t i = 0; i < counter; i++) {
		if (adv_short_name_cmp(data->data,
				       data_len,
				       name_filter->name[i].target_name,
				       name_filter->name[i].min_len)) {

			control->filter_status.short_name.name =
				name_filter->name[i].target_name;
			control->filter_status.short_name.len = data_len;

			return true;
		}
	}

	return false;
}

static bool is_short_name_filter_enabled(void)
{
	return bt_scan.scan_filters.short_name.enabled;
}

static void short_name_check(struct bt_scan_control *control,
			     const struct bt_data *data)
{
	if (is_short_name_filter_enabled()) {
		if (adv_short_name_compare(data, control)) {
			control->filter_match_cnt++;

			/* Information about the filters matched. */
			control->filter_status.short_name.match = true;
			control->filter_match = true;
		}
	}
}

static int scan_short_name_filter_add(const struct bt_scan_short_name *short_name)
{
	uint8_t counter =
		bt_scan.scan_filters.short_name.cnt;
	struct bt_scan_short_name_filter *short_name_filter =
		    &bt_scan.scan_filters.short_name;
	uint8_t name_len;

	/* If no memory for filter. */
	if (counter >= CONFIG_BT_SCAN_SHORT_NAME_CNT) {
		return -ENOMEM;
	}

	name_len = strlen(short_name->name);

	/* Check the name length. */
	if ((name_len == 0) ||
	    (name_len > CONFIG_BT_SCAN_SHORT_NAME_MAX_LEN)) {
		return -EINVAL;
	}

	/* Check for duplicated filter. */
	for (size_t i = 0; i < counter; i++) {
		if (!strcmp(short_name_filter->name[i].target_name,
			    short_name->name)) {
			return 0;
		}
	}

	/* Add name to the filter. */
	short_name_filter->name[counter].min_len = short_name->min_len;
	memcpy(short_name_filter->name[counter].target_name,
	       short_name->name,
	       name_len);

	bt_scan.scan_filters.short_name.cnt++;

	LOG_DBG("Adding filter on %s name", short_name->name);

	return 0;
}

static bool find_uuid(const uint8_t *data,
		      uint8_t data_len,
		      uint8_t uuid_type,
		      const struct bt_scan_uuid *target_uuid)
{
	uint8_t uuid_len;

	switch (uuid_type) {
	case BT_UUID_TYPE_16:
		uuid_len = sizeof(uint16_t);
		break;

	case BT_UUID_TYPE_32:
		uuid_len = sizeof(uint32_t);
		break;

	case BT_UUID_TYPE_128:
		uuid_len = BT_SCAN_UUID_128_SIZE * sizeof(uint8_t);
		break;

	default:
		return false;
	}

	for (size_t i = 0; i < data_len; i += uuid_len) {
		struct bt_uuid_128 uuid;

		if (!bt_uuid_create(&uuid.uuid, &data[i], uuid_len)) {
			return false;
		}

		if (bt_uuid_cmp(&uuid.uuid, target_uuid->uuid) == 0) {
			return true;
		}
	}

	return false;
}

static bool adv_uuid_compare(const struct bt_data *data, uint8_t uuid_type,
			     struct bt_scan_control *control)
{
	const struct bt_scan_uuid_filter *uuid_filter =
			&bt_scan.scan_filters.uuid;
	const bool all_filters_mode = bt_scan.scan_filters.all_mode;
	const uint8_t counter = bt_scan.scan_filters.uuid.cnt;
	uint8_t data_len = data->data_len;
	uint8_t uuid_match_cnt = 0;

	for (size_t i = 0; i < counter; i++) {

		if (find_uuid(data->data, data_len, uuid_type,
			      &uuid_filter->uuid[i])) {
			control->filter_status.uuid.uuid[uuid_match_cnt] =
				uuid_filter->uuid[i].uuid;

			uuid_match_cnt++;

			/* In the normal filter mode,
			 * only one UUID is needed to match.
			 */
			if (!all_filters_mode) {
				break;
			}

		} else if (all_filters_mode) {
			break;
		}
	}

	control->filter_status.uuid.count = uuid_match_cnt;

	/* In the multifilter mode, all UUIDs must be found in
	 * the advertisement packets.
	 */
	if ((all_filters_mode && (uuid_match_cnt == counter)) ||
	    ((!all_filters_mode) && (uuid_match_cnt > 0))) {
		return true;
	}

	return false;
}

static bool is_uuid_filter_enabled(void)
{
	return bt_scan.scan_filters.uuid.enabled;
}

static void uuid_check(struct bt_scan_control *control,
		       const struct bt_data *data,
		       uint8_t type)
{
	if (is_uuid_filter_enabled()) {
		if (adv_uuid_compare(data, type, control)) {
			control->filter_match_cnt++;

			/* Information about the filters matched. */
			control->filter_status.uuid.match = true;
			control->filter_match = true;
		}
	}
}

static int scan_uuid_filter_add(struct bt_uuid *uuid)
{
	struct bt_scan_uuid *uuid_filter = bt_scan.scan_filters.uuid.uuid;
	uint8_t counter = bt_scan.scan_filters.uuid.cnt;
	struct bt_uuid_16 *uuid_16;
	struct bt_uuid_32 *uuid_32;
	struct bt_uuid_128 *uuid_128;

	/* If no memory. */
	if (counter >= CONFIG_BT_SCAN_UUID_CNT) {
		return -ENOMEM;
	}

	/* Check for duplicated filter. */
	for (size_t i = 0; i < counter; i++) {
		if (bt_uuid_cmp(uuid_filter[i].uuid, uuid) == 0) {
			return 0;
		}
	}

	/* Add UUID to the filter. */
	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		uuid_16 = BT_UUID_16(uuid);

		uuid_filter[counter].uuid_data.uuid_16 = *uuid_16;
		uuid_filter[counter].uuid =
				(struct bt_uuid *)&uuid_filter[counter].uuid_data.uuid_16;
		break;

	case BT_UUID_TYPE_32:
		uuid_32 = BT_UUID_32(uuid);

		uuid_filter[counter].uuid_data.uuid_32 = *uuid_32;
		uuid_filter[counter].uuid =
				(struct bt_uuid *)&uuid_filter[counter].uuid_data.uuid_32;
		break;

	case BT_UUID_TYPE_128:
		uuid_128 = BT_UUID_128(uuid);

		uuid_filter[counter].uuid_data.uuid_128 = *uuid_128;
		uuid_filter[counter].uuid =
				(struct bt_uuid *)&uuid_filter[counter].uuid_data.uuid_128;
		break;

	default:
		return -EINVAL;
	}

	bt_scan.scan_filters.uuid.cnt++;
	LOG_DBG("Added filter on UUID type %x", uuid->type);

	return 0;
}

static bool find_appearance(const uint8_t *data,
			    uint8_t data_len,
			    const uint16_t *appearance)
{
	if (data_len != sizeof(uint16_t)) {
		return false;
	}

	uint16_t decoded_appearance = sys_get_be16(data);

	if (decoded_appearance == *appearance) {
		return true;
	}

	/* Could not find the appearance among the encoded data. */
	return false;
}

static bool adv_appearance_compare(const struct bt_data *data,
				   struct bt_scan_control *control)
{
	const struct bt_scan_appearance_filter *appearance_filter =
			&bt_scan.scan_filters.appearance;
	const uint8_t counter =
			bt_scan.scan_filters.appearance.cnt;
	uint8_t data_len = data->data_len;

	/* Verify if the advertised appearance matches
	 * the provided appearance.
	 */
	for (size_t i = 0; i < counter; i++) {
		if (find_appearance(data->data,
				    data_len,
				    &appearance_filter->appearance[i])) {

			control->filter_status.appearance.appearance =
					&appearance_filter->appearance[i];

			return true;
		}
	}

	return false;
}

static bool is_appearance_filter_enabled(void)
{
	return bt_scan.scan_filters.appearance.enabled;
}

static void appearance_check(struct bt_scan_control *control,
			     const struct bt_data *data)
{
	if (is_appearance_filter_enabled()) {
		if (adv_appearance_compare(data, control)) {
			control->filter_match_cnt++;

			/* Information about the filters matched. */
			control->filter_status.appearance.match = true;
			control->filter_match = true;
		}
	}
}

static int scan_appearance_filter_add(uint16_t appearance)
{
	uint16_t *appearance_filter = bt_scan.scan_filters.appearance.appearance;
	uint8_t counter = bt_scan.scan_filters.appearance.cnt;

	/* If no memory. */
	if (counter >= CONFIG_BT_SCAN_APPEARANCE_CNT) {
		return -ENOMEM;
	}

	/* Check for duplicated filter. */
	for (size_t i = 0; i < counter; i++) {
		if (appearance_filter[i] == appearance) {
			return 0;
		}
	}

	/* Add appearance to the filter. */
	appearance_filter[counter] = appearance;
	bt_scan.scan_filters.appearance.cnt++;

	LOG_DBG("Added filter on appearance %x", appearance);

	return 0;
}

static bool adv_manufacturer_data_cmp(const uint8_t *data,
				      uint8_t data_len,
				      const uint8_t *target_data,
				      uint8_t target_data_len)
{
	if (target_data_len > data_len) {
		return false;
	}

	if (memcmp(target_data, data, target_data_len) != 0) {
		return false;
	}

	return true;
}

static bool adv_manufacturer_data_compare(const struct bt_data *data,
					  struct bt_scan_control *control)
{
	const struct bt_scan_manufacturer_data_filter *md_filter =
		&bt_scan.scan_filters.manufacturer_data;
	uint8_t counter = bt_scan.scan_filters.manufacturer_data.cnt;

	/* Compare the name found with the name filter. */
	for (size_t i = 0; i < counter; i++) {
		if (adv_manufacturer_data_cmp(data->data,
				data->data_len,
				md_filter->manufacturer_data[i].data,
				md_filter->manufacturer_data[i].data_len)) {

			control->filter_status.manufacturer_data.data =
				md_filter->manufacturer_data[i].data;
			control->filter_status.manufacturer_data.len =
				md_filter->manufacturer_data[i].data_len;

			return true;
		}
	}

	return false;
}

static bool is_manufacturer_data_filter_enabled(void)
{
	return bt_scan.scan_filters.manufacturer_data.enabled;
}

static void manufacturer_data_check(struct bt_scan_control *control,
				    const struct bt_data *data)
{
	if (is_manufacturer_data_filter_enabled()) {
		if (adv_manufacturer_data_compare(data, control)) {
			control->filter_match_cnt++;

			/* Information about the filters matched. */
			control->filter_status.manufacturer_data.match = true;
			control->filter_match = true;
		}
	}
}

static int scan_manufacturer_data_filter_add(const struct bt_scan_manufacturer_data *manufacturer_data)
{
	struct bt_scan_manufacturer_data_filter *md_filter =
		&bt_scan.scan_filters.manufacturer_data;
	uint8_t counter = bt_scan.scan_filters.manufacturer_data.cnt;

	/* If no memory for filter. */
	if (counter >= CONFIG_BT_SCAN_MANUFACTURER_DATA_CNT) {
		return -ENOMEM;
	}

	/* Check the data length. */
	if ((manufacturer_data->data_len == 0) ||
			(manufacturer_data->data_len >
			 CONFIG_BT_SCAN_MANUFACTURER_DATA_MAX_LEN)) {
		return -EINVAL;
	}

	/* Check for duplicated filter. */
	for (size_t i = 0; i < counter; i++) {
		if (adv_manufacturer_data_cmp(manufacturer_data->data,
				manufacturer_data->data_len,
				md_filter->manufacturer_data[i].data,
				md_filter->manufacturer_data[i].data_len)) {
			return 0;
		}
	}

	/* Add manufacturer data to filter. */
	memcpy(md_filter->manufacturer_data[counter].data,
			manufacturer_data->data, manufacturer_data->data_len);
	md_filter->manufacturer_data[counter].data_len =
		manufacturer_data->data_len;

	bt_scan.scan_filters.manufacturer_data.cnt++;

	LOG_DBG("Adding filter on manufacturer data");

	return 0;
}

static bool check_filter_mode(uint8_t mode)
{
	return (mode & MODE_CHECK) != 0;
}

static void scan_default_param_set(void)
{
	struct bt_le_scan_param *scan_param = BT_LE_SCAN_PASSIVE;

	/* Set the default parameters. */
	bt_scan.scan_param = *scan_param;
}

#if CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER
static void connected(struct bt_conn *conn, uint8_t err)
{
	scan_attempts_filter_device_add(bt_conn_get_dst(conn));
	if (err) {
		device_conn_attempts_count(conn);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	device_conn_attempts_count(conn);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected
};
#endif /* CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER */

static void scan_default_conn_param_set(void)
{
	struct bt_le_conn_param *conn_param = BT_LE_CONN_PARAM_DEFAULT;

	/* Set default Connection params. */
	bt_scan.conn_param = *conn_param;
}

int bt_scan_filter_add(enum bt_scan_filter_type type,
		       const void *data)
{
	char *name;
	struct bt_scan_short_name *short_name;
	bt_addr_le_t *addr;
	struct bt_uuid *uuid;
	uint16_t appearance;
	struct bt_scan_manufacturer_data *manufacturer_data;
	int err = 0;

	if (!data) {
		return -EINVAL;
	}

	k_mutex_lock(&scan_mutex, K_FOREVER);

	switch (type) {
	case BT_SCAN_FILTER_TYPE_NAME:
		name = (char *)data;
		err = scan_name_filter_add(name);
		break;

	case BT_SCAN_FILTER_TYPE_SHORT_NAME:
		short_name = (struct bt_scan_short_name *)data;
		err = scan_short_name_filter_add(short_name);
		break;

	case BT_SCAN_FILTER_TYPE_ADDR:
		addr = (bt_addr_le_t *)data;
		err = scan_addr_filter_add(addr);
		break;

	case BT_SCAN_FILTER_TYPE_UUID:
		uuid = (struct bt_uuid *)data;
		err = scan_uuid_filter_add(uuid);
		break;

	case BT_SCAN_FILTER_TYPE_APPEARANCE:
		appearance = *((uint16_t *)data);
		err = scan_appearance_filter_add(appearance);
		break;

	case BT_SCAN_FILTER_TYPE_MANUFACTURER_DATA:
		manufacturer_data = (struct bt_scan_manufacturer_data *)data;
		err = scan_manufacturer_data_filter_add(manufacturer_data);
		break;

	default:
		err = -EINVAL;
		break;
	}

	k_mutex_unlock(&scan_mutex);

	return err;
}

void bt_scan_filter_remove_all(void)
{
	k_mutex_lock(&scan_mutex, K_FOREVER);

	struct bt_scan_name_filter *name_filter =
			&bt_scan.scan_filters.name;
	name_filter->cnt = 0;

	struct bt_scan_short_name_filter *short_name_filter =
			&bt_scan.scan_filters.short_name;
	short_name_filter->cnt = 0;

	struct bt_scan_addr_filter *addr_filter =
			&bt_scan.scan_filters.addr;
	addr_filter->cnt = 0;

	struct bt_scan_uuid_filter *uuid_filter =
			&bt_scan.scan_filters.uuid;
	uuid_filter->cnt = 0;

	struct bt_scan_appearance_filter *appearance_filter =
			&bt_scan.scan_filters.appearance;
	appearance_filter->cnt = 0;

	struct bt_scan_manufacturer_data_filter *manufacturer_data_filter =
		&bt_scan.scan_filters.manufacturer_data;
	manufacturer_data_filter->cnt = 0;

	k_mutex_unlock(&scan_mutex);
}

void bt_scan_filter_disable(void)
{
	/* Disable all filters. */
	bt_scan.scan_filters.name.enabled = false;
	bt_scan.scan_filters.short_name.enabled = false;
	bt_scan.scan_filters.addr.enabled = false;
	bt_scan.scan_filters.uuid.enabled = false;
	bt_scan.scan_filters.appearance.enabled = false;
	bt_scan.scan_filters.manufacturer_data.enabled = false;
}

int bt_scan_filter_enable(uint8_t mode, bool match_all)
{
	/* Check if the mode is correct. */
	if (!check_filter_mode(mode)) {
		return -EINVAL;
	}

	/* Disable filters. */
	bt_scan_filter_disable();

	struct bt_scan_filters *filters = &bt_scan.scan_filters;

	/* Turn on the filters of your choice. */
	if (mode & BT_SCAN_ADDR_FILTER) {
		filters->addr.enabled = true;
	}

	if (mode & BT_SCAN_NAME_FILTER) {
		filters->name.enabled = true;
	}

	if (mode & BT_SCAN_SHORT_NAME_FILTER) {
		filters->short_name.enabled = true;
	}

	if (mode & BT_SCAN_UUID_FILTER) {
		filters->uuid.enabled = true;
	}

	if (mode & BT_SCAN_APPEARANCE_FILTER) {
		filters->appearance.enabled = true;
	}

	if (mode & BT_SCAN_MANUFACTURER_DATA_FILTER) {
		filters->manufacturer_data.enabled = true;
	}

	/* Select the filter mode. */
	filters->all_mode = match_all;

	return 0;
}

int bt_scan_filter_get(struct bt_filter_status *status)
{
	if (!status) {
		return -EINVAL;
	}

	status->addr.enabled = bt_scan.scan_filters.addr.enabled;
	status->addr.cnt = bt_scan.scan_filters.addr.cnt;
	status->name.enabled = bt_scan.scan_filters.name.enabled;
	status->name.cnt = bt_scan.scan_filters.name.cnt;
	status->short_name.enabled =
			bt_scan.scan_filters.short_name.enabled;
	status->short_name.cnt = bt_scan.scan_filters.short_name.cnt;
	status->uuid.enabled = bt_scan.scan_filters.uuid.enabled;
	status->uuid.cnt = bt_scan.scan_filters.uuid.cnt;
	status->appearance.enabled =
			bt_scan.scan_filters.appearance.enabled;
	status->appearance.cnt =
			bt_scan.scan_filters.appearance.cnt;
	status->manufacturer_data.cnt =
			bt_scan.scan_filters.manufacturer_data.cnt;

	return 0;
}

int bt_scan_stop(void)
{
	return bt_le_scan_stop();
}

static struct bt_le_scan_cb scan_cb;
void bt_scan_init(const struct bt_scan_init_param *init)
{
	bt_le_scan_cb_register(&scan_cb);

	/* Disable all scanning filters. */
	memset(&bt_scan.scan_filters, 0, sizeof(bt_scan.scan_filters));

	/* If the pointer to the initialization structure exist,
	 * use it to scan the configuration.
	 */
	if (init) {
		bt_scan.connect_if_match = init->connect_if_match;

		if (init->scan_param) {
			bt_scan.scan_param = *init->scan_param;
		} else {
			/* Use the default static configuration. */
			scan_default_param_set();
		}

		if (init->conn_param) {
			bt_scan.conn_param = *init->conn_param;
		} else {
			/* Use the default static configuration. */
			scan_default_conn_param_set();
		}
	} else {
		/* If pointer is NULL, use the static default configuration. */
		scan_default_param_set();
		scan_default_conn_param_set();

		bt_scan.connect_if_match = false;
	}

#if CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER
	bt_conn_cb_register(&conn_callbacks);
#endif /* CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER */
}

void bt_scan_update_init_conn_params(struct bt_le_conn_param *new_conn_param)
{
	bt_scan.conn_param = *new_conn_param;
}

static void check_enabled_filters(struct bt_scan_control *control)
{
	control->filter_cnt = 0;

	if (is_addr_filter_enabled()) {
		control->filter_cnt++;
	}

	if (is_name_filter_enabled()) {
		control->filter_cnt++;
	}

	if (is_short_name_filter_enabled()) {
		control->filter_cnt++;
	}

	if (is_uuid_filter_enabled()) {
		control->filter_cnt++;
	}

	if (is_appearance_filter_enabled()) {
		control->filter_cnt++;
	}

	if (is_manufacturer_data_filter_enabled()) {
		control->filter_cnt++;
	}
}

static bool adv_data_found(struct bt_data *data, void *user_data)
{
	struct bt_scan_control *scan_control =
			(struct bt_scan_control *)user_data;

	switch (data->type) {
	case BT_DATA_NAME_COMPLETE:
		/* Check the name filter. */
		name_check(scan_control, data);
		break;

	case BT_DATA_NAME_SHORTENED:
		/* Check the short name filter. */
		short_name_check(scan_control, data);
		break;

	case BT_DATA_GAP_APPEARANCE:
		/* Check the appearance filter. */
		appearance_check(scan_control, data);
		break;

	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		/* Check the UUID filter. */
		uuid_check(scan_control, data, BT_UUID_TYPE_16);
		break;

	case BT_DATA_UUID32_SOME:
	case BT_DATA_UUID32_ALL:
		uuid_check(scan_control, data, BT_UUID_TYPE_32);
		break;

	case BT_DATA_UUID128_SOME:
	case BT_DATA_UUID128_ALL:
		/* Check the UUID filter. */
		uuid_check(scan_control, data, BT_UUID_TYPE_128);
		break;

	case BT_DATA_MANUFACTURER_DATA:
		/* Check the manufacturer data filter. */
		manufacturer_data_check(scan_control, data);
		break;

	default:
		break;
	}

	return true;
}

static void filter_state_check(struct bt_scan_control *control,
			       const bt_addr_le_t *addr)
{
	if (!scan_device_filter_check(addr)) {
		return;
	}

	if (control->all_mode &&
	    (control->filter_match_cnt == control->filter_cnt)) {
		notify_filter_matched(&control->device_info,
				      &control->filter_status,
				      control->connectable);
		scan_connect_with_target(control, addr);
	}

	/* In the normal filter mode, only one filter match is
	 * needed to generate the notification to the main application.
	 */
	else if ((!control->all_mode) && control->filter_match) {
		notify_filter_matched(&control->device_info,
				      &control->filter_status,
				      control->connectable);
		scan_connect_with_target(control, addr);
	} else {
		notify_filter_no_match(&control->device_info,
				       control->connectable);
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *ad)
{
	struct bt_scan_control scan_control;
	struct net_buf_simple_state state;

	memset(&scan_control, 0, sizeof(scan_control));

	scan_control.all_mode = bt_scan.scan_filters.all_mode;

	check_enabled_filters(&scan_control);

	/* Check id device is connectable. */
	scan_control.connectable =
		(info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0;

	/* Check the address filter. */
	check_addr(&scan_control, info->addr);

	/* Save advertising buffer state to transfer it
	 * data to application if futher processing is needed.
	 */
	net_buf_simple_save(ad, &state);
	bt_data_parse(ad, adv_data_found, (void *)&scan_control);
	net_buf_simple_restore(ad, &state);

	scan_control.device_info.recv_info = info;
	scan_control.device_info.conn_param = &bt_scan.conn_param;
	scan_control.device_info.adv_data = ad;

	/* In the multifilter mode, the number of the active filters must equal
	 * the number of the filters matched to generate the notification.
	 * If the event handler is not NULL, notify the main application.
	 */
	filter_state_check(&scan_control, info->addr);
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv,
};

int bt_scan_start(enum bt_scan_type scan_type)
{
	switch (scan_type) {
	case BT_SCAN_TYPE_SCAN_ACTIVE:
		bt_scan.scan_param.type = BT_LE_SCAN_TYPE_ACTIVE;
		break;

	case BT_SCAN_TYPE_SCAN_PASSIVE:
		bt_scan.scan_param.type = BT_LE_SCAN_TYPE_PASSIVE;
		break;

	default:
		return -EINVAL;
	}

	/* Start the scanning. */
	int err = bt_le_scan_start(&bt_scan.scan_param, NULL);

	if (!err) {
		LOG_DBG("Scanning");
	}

	return err;
}

int bt_scan_params_set(struct bt_le_scan_param *scan_param)
{
	bt_scan_stop();

	if (scan_param) {
		/* Assign new scanning parameters. */
		bt_scan.scan_param = *scan_param;
	} else {
		/* If NULL, use the default static configuration. */
		scan_default_param_set();
	}

	LOG_DBG("Scanning parameters have been changed successfully");

	return 0;
}

#if CONFIG_BT_SCAN_BLOCKLIST
int bt_scan_blocklist_device_add(const bt_addr_le_t *addr)
{
	int err = 0;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (!addr) {
		return -EINVAL;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	k_mutex_lock(&scan_mutex, K_FOREVER);

	/* Check if the device is already on the blocklist. */
	for (size_t i = 0; i < ARRAY_SIZE(bt_scan.blocklist.addr); i++) {
		if (bt_addr_le_cmp(&bt_scan.blocklist.addr[i], addr) == 0) {
			LOG_DBG("Device %s is already on the blocklist",
				log_strdup(addr_str));

			goto out;
		}
	}

	if (bt_scan.blocklist.count >= ARRAY_SIZE(bt_scan.blocklist.addr)) {
		LOG_ERR("No place for the new device");
		err = -ENOMEM;
	} else {
		bt_addr_le_copy(&bt_scan.blocklist.addr[bt_scan.blocklist.count],
				addr);
		bt_scan.blocklist.count++;
		LOG_INF("Device %s added to the scanning blocklist",
			log_strdup(addr_str));
	}

out:
	k_mutex_unlock(&scan_mutex);

	return err;
}

void bt_scan_blocklist_clear(void)
{
	k_mutex_lock(&scan_mutex, K_FOREVER);
	memset(&bt_scan.blocklist, 0, sizeof(bt_scan.blocklist));
	k_mutex_unlock(&scan_mutex);
}
#endif /* CONFIG_BT_SCAN_BLOCKLIST */

#if CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER
void bt_scan_conn_attempts_filter_clear(void)
{
	k_mutex_lock(&scan_mutex, K_FOREVER);
	memset(&bt_scan.attempts_filter, 0, sizeof(bt_scan.attempts_filter));
	k_mutex_unlock(&scan_mutex);
}
#endif /* CONFIG_BT_SCAN_CONN_ATTEMPTS_FILTER */
