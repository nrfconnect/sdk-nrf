/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <strings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>
#include <zephyr/bluetooth/hci.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log_ctrl.h>

#include "net/nrf_cloud.h"
#include "ble.h"

#include "ble_codec.h"
#include "gateway.h"
#include "ctype.h"
#include "nrf_cloud_transport.h"
#include "ble_conn_mgr.h"

#define SEND_NOTIFY_STACK_SIZE 2048
#define SEND_NOTIFY_PRIORITY 5
#define SUBSCRIPTION_LIMIT 16
#define NOTIFICATION_QUEUE_LIMIT 10
#define MAX_BUF_SIZE 11000
#define STR(x) #x
#define BT_UUID_GATT_CCC_VAL_STR STR(BT_UUID_GATT_CCC_VAL)

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble, CONFIG_NRFCLOUD_BLE_GATEWAY_LOG_LEVEL);

static char buffer[MAX_BUF_SIZE];
static struct gw_msg output = {
	.data.ptr = buffer,
	.data.len = 0,
	.data_max_len = MAX_BUF_SIZE
};

static bool discover_in_progress;
static bool scan_waiting;
static bool print_scan_results;

static int num_devices_found;
static int num_names_found;

struct k_timer rec_timer;
struct k_timer scan_timer;
struct k_timer auto_conn_start_timer;

struct k_work scan_off_work;
struct k_work ble_device_encode_work;
struct k_work start_auto_conn_work;

static atomic_t queued_notifications;

struct ble_scanned_dev ble_scanned_devices[MAX_SCAN_RESULTS];

/* Must be statically allocated */
/* TODO: The array needs to remain the entire time the sub exists.
 * Should probably be stored with the conn manager.
 */
static struct bt_gatt_subscribe_params sub_param[BT_MAX_SUBSCRIBES];
/* Array of connections corresponding to subscriptions above */
static struct bt_conn *sub_conn[BT_MAX_SUBSCRIBES];
static uint16_t sub_value[BT_MAX_SUBSCRIBES];
static uint8_t curr_subs;
static int next_sub_index;

static notification_cb_t notify_callback;

struct rec_data_t {
	void *fifo_reserved;
	struct ble_device_conn *connected_ptr;
	struct bt_gatt_subscribe_params sub_params;
	struct bt_gatt_read_params read_params;
	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	uint8_t data[256];
	bool read;
	uint16_t length;
};

K_FIFO_DEFINE(rec_fifo);

struct gatt_read_cache_entry {
	uint16_t handle;
	struct ble_device_conn *connected_ptr;
	const struct bt_conn *conn;
	struct uuid_handle_pair *uhp;
	bool pending;
	bool last;
} gatt_read_cache[CONFIG_BT_MAX_CONN];

/* Convert ble address string to uppcase */
void bt_to_upper(char *ble_mac_addr_str, uint8_t addr_len)
{
	for (int i = 0; i < addr_len; i++) {
		ble_mac_addr_str[i] = toupper(ble_mac_addr_str[i]);
	}
}

/* Get uuid string from bt_uuid object */
void bt_uuid_get_str(const struct bt_uuid *uuid, char *str, size_t len)
{
	uint32_t tmp1, tmp5;
	uint16_t tmp0, tmp2, tmp3, tmp4;

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		snprintk(str, len, "%04x", BT_UUID_16(uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		snprintk(str, len, "%08x", BT_UUID_32(uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		memcpy(&tmp0, &BT_UUID_128(uuid)->val[0], sizeof(tmp0));
		memcpy(&tmp1, &BT_UUID_128(uuid)->val[2], sizeof(tmp1));
		memcpy(&tmp2, &BT_UUID_128(uuid)->val[6], sizeof(tmp2));
		memcpy(&tmp3, &BT_UUID_128(uuid)->val[8], sizeof(tmp3));
		memcpy(&tmp4, &BT_UUID_128(uuid)->val[10], sizeof(tmp4));
		memcpy(&tmp5, &BT_UUID_128(uuid)->val[12], sizeof(tmp5));

		snprintk(str, len, "%08x%04x%04x%04x%08x%04x",
			tmp5, tmp4, tmp3, tmp2, tmp1, tmp0);
		break;
	default:
		(void)memset(str, 0, len);
		return;
	}
}

static int svc_attr_data_add(const struct bt_gatt_service_val *gatt_service,
			      uint16_t handle, struct ble_device_conn *ble_conn_ptr)
{
	char str[UUID_STR_LEN];

	bt_uuid_get_str(gatt_service->uuid, str, sizeof(str));

	bt_to_upper(str, strlen(str));
	LOG_INF("Service %s", str);

	return ble_conn_mgr_add_uuid_pair(gatt_service->uuid, handle, 0, 0,
					  BT_ATTR_SERVICE, ble_conn_ptr, true);
}

static int chrc_attr_data_add(const struct bt_gatt_chrc *gatt_chrc,
			       struct ble_device_conn *ble_conn_ptr)
{
	uint16_t handle = gatt_chrc->value_handle;

	return ble_conn_mgr_add_uuid_pair(gatt_chrc->uuid, handle, 1,
					  gatt_chrc->properties, BT_ATTR_CHRC,
					  ble_conn_ptr, false);
}

static int ccc_attr_data_add(const struct bt_uuid *uuid, uint16_t handle,
			      struct ble_device_conn *ble_conn_ptr)
{
	LOG_DBG("\tHandle: %d", handle);

	return ble_conn_mgr_add_uuid_pair(uuid, handle, 2, 0, BT_ATTR_CCC,
					  ble_conn_ptr, false);
}

/* Add attributes to the connection manager objects */
static int attr_add(const struct bt_gatt_dm *dm,
	const struct bt_gatt_dm_attr *attr,
	struct ble_device_conn *ble_conn_ptr)
{
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(attr);
	const struct bt_gatt_chrc *gatt_chrc =
		bt_gatt_dm_attr_chrc_val(attr);
	int err = 0;

	if ((bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) == 0) ||
		(bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY) == 0)) {
		err = svc_attr_data_add(gatt_service, attr->handle,
					ble_conn_ptr);
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) == 0) {
		err = chrc_attr_data_add(gatt_chrc, ble_conn_ptr);
	} else if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) == 0) {
		err = ccc_attr_data_add(attr->uuid, attr->handle, ble_conn_ptr);
	}
	return err;
}

static void full_addr_to_mac_str(const char *raw_addr, char *ble_mac_addr_str)
{
	memcpy(ble_mac_addr_str, raw_addr, BT_ADDR_LE_DEVICE_LEN);
	ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN] = 0;
	bt_to_upper(ble_mac_addr_str, BT_ADDR_LE_DEVICE_LEN);
}

static void le_addr_to_mac_addr_str(const bt_addr_le_t *le_addr, char *ble_mac_addr_str)
{
	char raw_addr[BT_ADDR_LE_STR_LEN + 1];

	bt_addr_le_to_str(le_addr, raw_addr, BT_ADDR_LE_STR_LEN);
	full_addr_to_mac_str(raw_addr, ble_mac_addr_str);
}

static void conn_to_mac_addr_str(const struct bt_conn *conn, char *ble_mac_addr_str)
{
	return le_addr_to_mac_addr_str(bt_conn_get_dst(conn), ble_mac_addr_str);
}

static int ble_dm_data_add(struct bt_gatt_dm *dm)
{
	const struct bt_gatt_dm_attr *attr = NULL;
	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	struct ble_device_conn *ble_conn_ptr;
	struct bt_conn *conn;
	int err;

	conn = bt_gatt_dm_conn_get(dm);
	conn_to_mac_addr_str(conn, ble_mac_addr_str);
	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &ble_conn_ptr);
	if (err) {
		LOG_ERR("Connection not found for addr %s", ble_mac_addr_str);
		return err;
	}

	discover_in_progress = true;

	attr = bt_gatt_dm_service_get(dm);

	err = attr_add(dm, attr, ble_conn_ptr);
	if (err) {
		LOG_ERR("Unable to add attribute: %d", err);
		return err;
	}

	while (NULL != (attr = bt_gatt_dm_attr_next(dm, attr))) {
		err = attr_add(dm, attr, ble_conn_ptr);
		if (err) {
			LOG_ERR("Unable to add attribute: %d", err);
			return err;
		}
	}
	return 0;
}

void ble_register_notify_callback(notification_cb_t callback)
{
	notify_callback = callback;
}

/* Thread responsible for transferring ble data over MQTT */
static void send_ble_read_data(int unused1, int unused2, int unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);
	char uuid[BT_UUID_STR_LEN];
	char path[BT_MAX_PATH_LEN];

	memset(uuid, 0, BT_UUID_STR_LEN);
	memset(path, 0, BT_MAX_PATH_LEN);

	struct ble_device_conn *connected_ptr;

	while (1) {
		int err;
		uint16_t handle;
		struct rec_data_t *rx_data = k_fifo_get(&rec_fifo, K_FOREVER);

		if (rx_data == NULL) {
			continue;
		}
		atomic_dec(&queued_notifications);
		LOG_DBG("Queued: %ld", atomic_get(&queued_notifications));

		connected_ptr = rx_data->connected_ptr;
		if (connected_ptr == NULL) {
			LOG_ERR("Bad connected_ptr!");
			goto cleanup;
		}

		if (rx_data->read) {
			handle = rx_data->read_params.single.handle;
			LOG_INF("Dequeued gatt_read results, addr %s handle %d",
				rx_data->ble_mac_addr_str, handle);
		} else {
			handle = rx_data->sub_params.value_handle;
			LOG_DBG("Dequeued BLE notification data, addr %s handle %d",
				rx_data->ble_mac_addr_str, handle);
		}
		/* LOG_HEXDUMP_DBG(rx_data->data, rx_data->length, "notify"); */

		err = ble_conn_mgr_get_uuid_by_handle(handle, uuid, connected_ptr);
		if (err) {
			if (discover_in_progress) {
				LOG_INF("Ignoring notification on %s due to BLE"
					" discovery in progress",
					rx_data->ble_mac_addr_str);
			} else {
				LOG_ERR("Unable to convert handle: %d", err);
			}
			goto cleanup;
		}

		bool ccc = !rx_data->read;

		if (strcmp(uuid, BT_UUID_GATT_CCC_VAL_STR) == 0) {
			ccc = true;
			handle--;
			LOG_INF("Force ccc for handle %u", handle);
		}

		err = ble_conn_mgr_generate_path(connected_ptr, handle, path, ccc);
		if (err) {
			LOG_ERR("Unable to generate path: %d", err);
			goto cleanup;
		}

		if (!rx_data->read && notify_callback) {
			err = notify_callback(rx_data->ble_mac_addr_str, uuid,
					      rx_data->data, rx_data->length);
			if (err) {
				/* callback should return 0 if it did not
				 * process the data
				 */
				goto cleanup;
			}
		}
		if (connected_ptr && connected_ptr->hidden) {
			LOG_DBG("Suppressing notification write to cloud");
			goto cleanup;
		}

		k_mutex_lock(&output.lock, K_FOREVER);
		if (rx_data->read && !ccc) {
			err = device_chrc_read_encode(rx_data->ble_mac_addr_str,
				uuid, path, ((char *)rx_data->data),
				rx_data->length, &output);
		} else if (rx_data->read && ccc) {
			err = device_descriptor_value_encode(rx_data->ble_mac_addr_str,
							     BT_UUID_GATT_CCC_VAL_STR,
							     path,
							     ((char *)rx_data->data),
							     rx_data->length,
							     &output, false);
		} else {
			err = device_value_changed_encode(rx_data->ble_mac_addr_str,
				uuid, path, ((char *)rx_data->data),
				rx_data->length, &output);
		}
		if (err) {
			k_mutex_unlock(&output.lock);
			LOG_ERR("Unable to encode: %d", err);
			goto cleanup;
		}
		err = g2c_send(&output.data);
		k_mutex_unlock(&output.lock);
		if (err) {
			LOG_ERR("Unable to send: %d", err);
		}

cleanup:
		k_free(rx_data);
	}
}

K_THREAD_DEFINE(ble_rec_thread, SEND_NOTIFY_STACK_SIZE,
	send_ble_read_data, NULL, NULL, NULL,
	SEND_NOTIFY_PRIORITY, 0, 0);

static void discovery_completed(struct bt_gatt_dm *disc, void *ctx)
{
	LOG_DBG("Attribute count: %d", bt_gatt_dm_attr_cnt(disc));

	int err;

	err = ble_dm_data_add(disc);
	/* TBD: how can we cancel remainder of discovery if we get an error? */

	bt_gatt_dm_data_release(disc);

	bt_gatt_dm_continue(disc, NULL);
}

static void discovery_service_complete(struct bt_conn *conn, void *ctx)
{
	LOG_DBG("Discovery complete.");

	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	struct ble_device_conn *connected_ptr;
	int err;

	conn_to_mac_addr_str(conn, ble_mac_addr_str);
	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
	if (err) {
		LOG_ERR("Connection not found for addr %s", ble_mac_addr_str);
	} else {
		/* only set discovered true and send results if it seems we were
		 * successful at doing a full discovery
		 */
		if (connected_ptr->connected && connected_ptr->num_pairs) {
			connected_ptr->encode_discovered = true;
			connected_ptr->discovered = true;
		} else {
			LOG_WRN("Discovery not completed");
		}
		connected_ptr->discovering = false;
	}
	discover_in_progress = false;

	/* check scan waiting */
	if (scan_waiting) {
		scan_start(print_scan_results);
	}
}

static void discovery_error_found(struct bt_conn *conn, int err, void *ctx)
{
	LOG_ERR("The discovery procedure failed, err %d", err);

	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	struct ble_device_conn *connected_ptr;

	conn_to_mac_addr_str(conn, ble_mac_addr_str);
	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
	if (err) {
		LOG_ERR("Connection not found for addr %s", ble_mac_addr_str);
	} else {
		connected_ptr->num_pairs = 0;
		connected_ptr->discovering = false;
		connected_ptr->discovered = false;
	}
	discover_in_progress = false;

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		LOG_INF("Saving settings");
		settings_save();
	}

	/* Disconnect? */
	bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

	/* check scan waiting */
	if (scan_waiting) {
		scan_start(print_scan_results);
	}
}

static int find_gatt_read_cache_entry(const struct bt_conn *conn, uint16_t handle)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if ((gatt_read_cache[i].conn == conn) &&
		    (gatt_read_cache[i].handle == handle)) {
			return i;
		}
	}
	return -1;
}

static bool store_gatt_read_cache(struct ble_device_conn *connected_ptr,
				  const struct bt_conn *conn, uint16_t handle,
				  struct uuid_handle_pair *uhp)
{
	int entry = find_gatt_read_cache_entry(conn, handle);

	if (entry == -1) {
		for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			if (!gatt_read_cache[i].pending) {
				entry = i;
				break;
			}
		}
		if (entry == -1) {
			LOG_ERR("OUT OF GATT READ CACHE ENTRIES!");
			return false;
		}
	}
	struct gatt_read_cache_entry *c = &gatt_read_cache[entry];

	c->pending = true;
	c->connected_ptr = connected_ptr;
	c->conn = conn;
	c->handle = handle;
	c->uhp = uhp;
	c->last = false;

	return true;
}

static uint8_t gatt_read_callback(struct bt_conn *conn, uint8_t err,
				  struct bt_gatt_read_params *params,
				  const void *data, uint16_t length)
{
	int ret = BT_GATT_ITER_CONTINUE;
	int entry = find_gatt_read_cache_entry(conn, params->single.handle);

	if (entry == -1) {
		return BT_GATT_ITER_STOP;
	}
	struct gatt_read_cache_entry *c = &gatt_read_cache[entry];
	struct uuid_handle_pair *uhp = c->uhp;

	if ((length > 0) && (data != NULL)) {
		/* Store value so next discovery of this device sends the
		 * most recent value for each readable characteristic,
		 * rather than 0.
		 */
		uint16_t offset = MIN(params->single.offset, BT_MAX_VALUE_LEN);
		uint16_t this_len = MIN(length, (BT_MAX_VALUE_LEN - offset));

		uhp->value_len = offset + this_len;
		memcpy(&uhp->value[offset], data, this_len);
	}

	/* End of read */
	if ((data == NULL) || (length < 22)) {
		char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
		size_t size = sizeof(struct rec_data_t);
		struct rec_data_t *read_data = (struct rec_data_t *)k_malloc(size);

		if (read_data == NULL) {
			LOG_ERR("Out of memory error in gatt_read_callback(): "
				"%ld queued notifications",
				atomic_get(&queued_notifications));
			ret = BT_GATT_ITER_STOP;
			return ret;
		}

		conn_to_mac_addr_str(conn, ble_mac_addr_str);

		memset(&read_data->sub_params, 0, sizeof(read_data->sub_params));
		memcpy(&read_data->read_params, params, sizeof(struct bt_gatt_read_params));
		memcpy(&read_data->ble_mac_addr_str, ble_mac_addr_str, BT_ADDR_LE_DEVICE_LEN + 1);
		memcpy(&read_data->data, uhp->value, uhp->value_len);
		read_data->fifo_reserved = NULL;
		read_data->read = true;
		read_data->length = uhp->value_len;
		read_data->connected_ptr = c->connected_ptr;

		atomic_inc(&queued_notifications);
		k_fifo_put(&rec_fifo, read_data);
		LOG_DBG("Enqueued gatt_read of %d bytes; queued:%ld",
			read_data->length, atomic_get(&queued_notifications));
		c->pending = false;

		ret = BT_GATT_ITER_STOP;
	}

	return ret;
}

static int gatt_read_handle(struct ble_device_conn *connected_ptr,
			    struct uuid_handle_pair *uhp, bool ccc)
{
	int err;
	static struct bt_gatt_read_params params;
	struct bt_conn *conn;
	uint16_t handle = uhp->handle;

	if (ccc) {
		handle++;
	}
	params.handle_count = 1;
	params.single.handle = handle;
	params.single.offset = 0;
	params.func = gatt_read_callback;

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &connected_ptr->bt_addr);
	if (conn == NULL) {
		LOG_ERR("Null Conn object");
		return -EINVAL;
	}

	(void)store_gatt_read_cache(connected_ptr, conn, handle, uhp);
	err = bt_gatt_read(conn, &params);
	bt_conn_unref(conn);
	return err;
}

int gatt_read(const char *ble_mac_addr_str, const char *chrc_uuid, bool ccc)
{
	int err;
	struct ble_device_conn *connected_ptr;
	struct uuid_handle_pair *uhp;

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
	if (err) {
		return err;
	}

	uhp = ble_conn_mgr_get_uhp_by_uuid(chrc_uuid, connected_ptr);
	if (!uhp) {
		return 1;
	}

	LOG_DBG("Read %s, 0x%02X, %d", chrc_uuid,
		ccc ? uhp->handle : uhp->handle + 1, ccc);
	return gatt_read_handle(connected_ptr, uhp, ccc);
}

static void on_sent(struct bt_conn *conn, uint8_t err,
		    struct bt_gatt_write_params *params)
{
	ARG_UNUSED(conn);

	LOG_DBG("Sent Data of Length: %d", params->length);
}

int gatt_write(const char *ble_mac_addr_str, const char *chrc_uuid, uint8_t *data,
	       uint16_t data_len, bt_gatt_write_func_t cb)
{
	int err;
	struct bt_conn *conn;
	struct ble_device_conn *connected_ptr;
	uint16_t handle;
	static struct bt_gatt_write_params params;

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
	if (err) {
		return err;
	}

	err = ble_conn_mgr_get_handle_by_uuid(&handle, chrc_uuid,
					      connected_ptr);
	if (err) {
		return err;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &connected_ptr->bt_addr);
	if (conn == NULL) {
		LOG_ERR("Null Conn object");
		return -EINVAL;
	}

	LOG_DBG("Writing to addr: %s to chrc %s with handle %d",
		ble_mac_addr_str, chrc_uuid, handle);
	LOG_HEXDUMP_DBG(data, data_len, "Data to write");

	params.func = cb ? cb : on_sent;
	params.handle = handle;
	params.offset = 0;
	params.data = data;
	params.length = data_len;

	err = bt_gatt_write(conn, &params);
	bt_conn_unref(conn);
	return err;
}

int gatt_write_without_response(const char *ble_mac_addr_str, const char *chrc_uuid,
				uint8_t *data, uint16_t data_len)
{
	int err;

	struct bt_conn *conn;
	struct ble_device_conn *connected_ptr;
	uint16_t handle;
	/* static struct bt_gatt_write_params params; */

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
	if (err) {
		return err;
	}

	err = ble_conn_mgr_get_handle_by_uuid(&handle, chrc_uuid,
					      connected_ptr);
	if (err) {
		return err;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &connected_ptr->bt_addr);
	if (conn == NULL) {
		LOG_ERR("Null Conn object");
		return -EINVAL;
	}

	LOG_DBG("Writing w/o resp to addr: %s to chrc %s with handle %d",
		ble_mac_addr_str, chrc_uuid, handle);
	LOG_HEXDUMP_DBG(data, data_len, "Data to write");

	err = bt_gatt_write_without_response(conn, handle, data, data_len,
					     false);
	bt_conn_unref(conn);
	return err;
}

static uint8_t on_notification_received(struct bt_conn *conn,
					struct bt_gatt_subscribe_params *params,
					const void *data, uint16_t length)
{
	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	int ret = BT_GATT_ITER_CONTINUE;

	if (!data) {
		return BT_GATT_ITER_STOP;
	}

	if ((length > 0) && (data != NULL)) {
		conn_to_mac_addr_str(conn, ble_mac_addr_str);
		struct ble_device_conn *connected_ptr;
		int err;

		err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
		if (err) {
			return err;
		}

		struct rec_data_t tx_data = {
			.connected_ptr = connected_ptr,
			.read = false,
			.length = length
		};

		memcpy(&tx_data.ble_mac_addr_str, ble_mac_addr_str, BT_ADDR_LE_DEVICE_LEN + 1);
		memcpy(&tx_data.data, data, length);
		memcpy(&tx_data.sub_params, params,
			sizeof(struct bt_gatt_subscribe_params));

		size_t size = sizeof(struct rec_data_t);

		if (atomic_get(&queued_notifications) >=
		     NOTIFICATION_QUEUE_LIMIT) {
			struct rec_data_t *rx_data = k_fifo_get(&rec_fifo, K_NO_WAIT);

			LOG_INF("Dropping oldest message");
			if (rx_data != NULL) {
				uint16_t h;

				if (rx_data->read) {
					h = rx_data->read_params.single.handle;
				} else  {
					h = rx_data->sub_params.value_handle;
				}
				LOG_INF("Addr %s Handle %d Queued %ld",
					rx_data->ble_mac_addr_str,
					h,
					atomic_get(&queued_notifications));
				k_free(rx_data);
			}
			atomic_dec(&queued_notifications);
		}

		char *mem_ptr = k_malloc(size);

		if (mem_ptr == NULL) {
			LOG_ERR("Out of memory error in on_received(): "
				"%ld queued notifications",
				atomic_get(&queued_notifications));
			ret = BT_GATT_ITER_STOP;
		} else {
			atomic_inc(&queued_notifications);
			memcpy(mem_ptr, &tx_data, size);
			k_fifo_put(&rec_fifo, mem_ptr);
			LOG_DBG("Enqueued BLE notification data %ld",
				atomic_get(&queued_notifications));
		}
	}

	return ret;
}

static void send_sub(const char *ble_mac_addr_str, const char *path,
		     struct gw_msg *out, uint8_t *value)
{
	k_mutex_lock(&out->lock, K_FOREVER);
	device_descriptor_value_encode(ble_mac_addr_str,
				       BT_UUID_GATT_CCC_VAL_STR,
				       path, value, sizeof(value),
				       out, true);
	g2c_send(&out->data);
	device_value_write_result_encode(ble_mac_addr_str,
					 BT_UUID_GATT_CCC_VAL_STR,
					 path, value, sizeof(value),
					 out);
	g2c_send(&out->data);
	k_mutex_unlock(&out->lock);
}

int ble_subscribe(const char *ble_mac_addr_str, const char *chrc_uuid, uint8_t value_type)
{
	int err;
	char path[BT_MAX_PATH_LEN];
	struct bt_conn *conn = NULL;
	struct ble_device_conn *connected_ptr;
	uint16_t handle;
	bool subscribed;
	uint8_t param_index;

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
	if (err) {
		LOG_ERR("Connection not found for addr %s", ble_mac_addr_str);
		return -ENOLINK;
	}

	ble_conn_mgr_get_handle_by_uuid(&handle, chrc_uuid, connected_ptr);

	ble_conn_mgr_get_subscribed(handle, connected_ptr, &subscribed,
				    &param_index);

	if (connected_ptr->connected) {
		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &connected_ptr->bt_addr);
		if (conn == NULL) {
			/* work around strange error on Flic button --
			 * type changes when it should not
			 */
			connected_ptr->bt_addr.type = 0;
			conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &connected_ptr->bt_addr);
			if (conn == NULL) {
				LOG_ERR("Null Conn object");
				err = -EINVAL;
				goto end;
			}
		}
	}

	err = ble_conn_mgr_generate_path(connected_ptr, handle, path, true);
	if (err) {
		return err;
	}

	if (subscribed && (value_type == 0)) {
		/* If subscribed then unsubscribe. */
		if (conn != NULL) {
			bt_gatt_unsubscribe(conn, &sub_param[param_index]);
		}
		ble_conn_mgr_remove_subscribed(handle, connected_ptr);

		uint8_t value[2] = {0, 0};

		if (connected_ptr && connected_ptr->hidden) {
			LOG_DBG("suppressing value_changed");
		} else {
			send_sub(ble_mac_addr_str, path, &output, value);
		}
		LOG_INF("Unsubscribe: Addr %s Handle %d",
			ble_mac_addr_str, handle);

		sub_value[param_index] = 0;
		sub_conn[param_index] = NULL;
		if (curr_subs) {
			curr_subs--;
		}
	} else if (subscribed && (value_type != 0)) {
		uint8_t value[2] = {
			value_type,
			0
		};

		if (connected_ptr && connected_ptr->hidden) {
			LOG_DBG("suppressing value_changed");
		} else {
			send_sub(ble_mac_addr_str, path, &output, value);
		}
		LOG_INF("Subscribe Dup: Addr %s Handle %d %s",
			ble_mac_addr_str, handle,
			(value_type == BT_GATT_CCC_NOTIFY) ?
			"Notify" : "Indicate");

	} else if (value_type == 0) {
		LOG_DBG("Unsubscribe N/A: Addr %s Handle %d",
			ble_mac_addr_str, handle);
	} else if (curr_subs < SUBSCRIPTION_LIMIT) {
		if (conn != NULL) {
			sub_param[next_sub_index].notify = on_notification_received;
			sub_param[next_sub_index].value = value_type;
			sub_param[next_sub_index].value_handle = handle;
			sub_param[next_sub_index].ccc_handle = handle + 1;
			sub_value[next_sub_index] = value_type;
			sub_conn[next_sub_index] = conn;
			err = bt_gatt_subscribe(conn, &sub_param[next_sub_index]);
			if (err) {
				LOG_ERR("Subscribe failed (err %d)", err);
				goto end;
			}
		}
		ble_conn_mgr_set_subscribed(handle, next_sub_index,
					    connected_ptr);

		uint8_t value[2] = {
			value_type,
			0
		};

		if (connected_ptr && connected_ptr->hidden) {
			LOG_DBG("suppressing value_changed");
		} else {
			send_sub(ble_mac_addr_str, path, &output, value);
		}
		LOG_INF("Subscribe: Addr %s Handle %d %s",
			ble_mac_addr_str, handle,
			(value_type == BT_GATT_CCC_NOTIFY) ?
			"Notify" : "Indicate");

		curr_subs++;
		next_sub_index++;
	} else {
		char msg[64];

		sprintf(msg, "Reached subscription limit of %d",
			SUBSCRIPTION_LIMIT);

		/* Send error when limit is reached. */
		k_mutex_lock(&output.lock, K_FOREVER);
		device_error_encode(ble_mac_addr_str, msg, &output);
		g2c_send(&output.data);
		k_mutex_unlock(&output.lock);
	}

end:
	if (conn != NULL) {
		bt_conn_unref(conn);
	}
	return err;
}

int ble_subscribe_handle(const char *ble_mac_addr_str, uint16_t handle, uint8_t value_type)
{
	char uuid[BT_UUID_STR_LEN];
	struct ble_device_conn *conn_ptr;
	int err;

	LOG_DBG("Addr %s handle %u value_type %u", ble_mac_addr_str,
		handle, (unsigned int)value_type);
	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &conn_ptr);
	if (err == 0) {
		err = ble_conn_mgr_get_uuid_by_handle(handle, uuid, conn_ptr);
		if (err == 0) {
			LOG_DBG("Subscribing uuid %s",
				uuid);
			err = ble_subscribe(ble_mac_addr_str, uuid, value_type);
		}
	}
	return err;
}

int ble_subscribe_all(const char *ble_mac_addr_str, uint8_t value_type)
{
	unsigned int i;
	struct ble_device_conn *conn_ptr;
	struct uuid_handle_pair *up;
	int err;

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &conn_ptr);
	if (err) {
		return err;
	}

	for (i = 0; i < conn_ptr->num_pairs; i++) {
		up = conn_ptr->uuid_handle_pairs[i];
		if (up == NULL) {
			continue;
		}
		if ((up->properties & BT_GATT_CHRC_NOTIFY) !=
		    BT_GATT_CHRC_NOTIFY) {
			continue;
		}
		err = ble_subscribe_handle(ble_mac_addr_str, up->handle, value_type);
		if (err) {
			break;
		}
	}
	return err;
}


static int ble_subscribe_device(struct bt_conn *conn, bool subscribe)
{
	uint16_t handle;
	int i;
	int count = 0;
	struct ble_device_conn *connected_ptr;
	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	int err;

	if (conn == NULL) {
		return -EINVAL;
	}
	conn_to_mac_addr_str(conn, ble_mac_addr_str);
	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ptr);
	if (err) {
		LOG_ERR("Connection not found for addr %s", ble_mac_addr_str);
		return -EINVAL;
	}

	for (i = 0; i < BT_MAX_SUBSCRIBES; i++) {
		if (conn == sub_conn[i]) {
			handle = sub_param[i].value_handle;
			if (subscribe && sub_value[i]) {
				sub_param[i].value = sub_value[i];
				bt_gatt_subscribe(conn, &sub_param[i]);
				ble_conn_mgr_set_subscribed(handle, i,
							    connected_ptr);
				LOG_INF("Subscribe: Addr %s Handle %d Idx %d",
					ble_mac_addr_str, handle, i);
				count++;
			} else {
				bt_gatt_unsubscribe(conn, &sub_param[i]);
				ble_conn_mgr_remove_subscribed(handle,
							       connected_ptr);
				sub_conn[i] = NULL;
				if (curr_subs) {
					curr_subs--;
				}
				LOG_INF("Unsubscribe: Addr %s Handle %d Idx %d",
					ble_mac_addr_str, handle, i);
				count++;
			}
		}
	}
	LOG_INF("Subscriptions changed for %d handles", count);
	return 0;
}

static struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed,
	.service_not_found = discovery_service_complete,
	.error_found = discovery_error_found,
};

int ble_discover(struct ble_device_conn *connection_ptr)
{
	int err = 0;
	char *ble_mac_addr_str = connection_ptr->ble_mac_addr_str;
	struct bt_conn *conn = NULL;

	LOG_INF("Discovering: %s\n", ble_mac_addr_str);

	if (!discover_in_progress) {

		conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &connection_ptr->bt_addr);
		if (conn == NULL) {
			LOG_DBG("ERROR: Null Conn object");
			return -EINVAL;
		}

		if (!connection_ptr->discovered) {
			if (connection_ptr->num_pairs) {
				LOG_INF("Marking device as discovered; "
					"num pairs = %u",
					connection_ptr->num_pairs);
				connection_ptr->discovering = false;
				connection_ptr->discovered = true;
				connection_ptr->encode_discovered = true;
				bt_conn_unref(conn);
				return 0;
			}
			discover_in_progress = true;
			connection_ptr->discovering = true;

			for (int i = 1; i < 3; i++) {
				if (i > 1) {
					LOG_INF("Retrying...");
				}
				err = bt_gatt_dm_start(conn, NULL, &discovery_cb, NULL);
				if (!err) {
					break;
				}
				LOG_WRN("Service discovery err %d", err);
				k_sleep(K_MSEC(500));
			}
			if (err) {
				LOG_ERR("Aborting discovery.  Disconnecting from device...");
				connection_ptr->discovering = false;
				discover_in_progress = false;
				bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				bt_conn_unref(conn);
				ble_conn_set_connected(connection_ptr, false);
				return err;
			}
		} else {
			connection_ptr->encode_discovered = true;
		}
	} else {
		return -EBUSY;
	}

	bt_conn_unref(conn);
	return err;
}

int disconnect_device_by_addr(const char *ble_mac_addr_str)
{
	int err;
	struct bt_conn *conn;
	bt_addr_le_t bt_addr;

	err = bt_addr_le_from_str(ble_mac_addr_str, "random", &bt_addr);
	if (err) {
		LOG_ERR("Address from string failed (err %d)", err);
		return err;
	}

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &bt_addr);
	if (conn == NULL) {
		LOG_DBG("ERROR: Null Conn object");
		return -EINVAL;
	}

	/* cloud commanded this, so remove any notifications or indications */
	err = ble_subscribe_device(conn, false);
	if (err) {
		LOG_ERR("Error unsubscribing device: %d", err);
	}

	/* Disconnect device. */
	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		LOG_ERR("Error disconnecting: %d", err);
	}
	bt_conn_unref(conn);

	return err;
}

int set_shadow_ble_conn(const char *ble_mac_addr_str, bool connecting, bool connected)
{
	int err;

	LOG_INF("Connecting=%u, connected=%u", connecting, connected);
	k_mutex_lock(&output.lock, K_FOREVER);
	err = device_shadow_data_encode(ble_mac_addr_str, connecting, connected,
					&output);
	if (!err) {
		err = gw_shadow_publish(&output.data);
		if (err) {
			LOG_ERR("nrf_cloud_gw_shadow_publish() failed %d", err);
		}
	} else {
		LOG_ERR("device_shadow_data_encode() failed %d", err);
	}
	k_mutex_unlock(&output.lock);
	return err;
}

int set_shadow_desired_conn(const struct desired_conn *desired, int num_desired)
{
	int err;

	k_mutex_lock(&output.lock, K_FOREVER);
	err = gateway_desired_list_encode(desired,
					  num_desired,
					  &output);
	if (!err) {
		err = gw_shadow_publish(&output.data);
		if (err) {
			LOG_ERR("gw_shadow_publish() failed %d", err);
		}
	} else {
		LOG_ERR("Failed %d", err);
	}
	k_mutex_unlock(&output.lock);

	return err;
}

static void auto_conn_start_work_handler(struct k_work *work)
{
	int err;

	/* Restart to scanning for allowlisted devices */
	struct bt_conn_le_create_param param = BT_CONN_LE_CREATE_PARAM_INIT(
		BT_CONN_LE_OPT_NONE,
		BT_GAP_SCAN_FAST_INTERVAL,
		BT_GAP_SCAN_FAST_WINDOW);

	err = bt_conn_le_create_auto(&param, BT_LE_CONN_PARAM_DEFAULT);

	if (err == -EALREADY) {
		LOG_INF("Auto connect already running");
	} else if (err == -EINVAL) {
		LOG_INF("Auto connect not necessary or invalid");
	} else if (err == -EAGAIN) {
		LOG_WRN("Device not ready; try starting auto connect later");
		k_timer_start(&auto_conn_start_timer, K_SECONDS(3), K_SECONDS(0));
	} else if (err == -ENOMEM) {
		LOG_ERR("Out of memory enabling auto connect");
	} else if (err) {
		LOG_ERR("Error enabling auto connect: %d", err);
	} else {
		LOG_INF("Auto connect started");
	}
}

K_WORK_DEFINE(auto_conn_start_work, auto_conn_start_work_handler);

static void auto_conn_start_timer_handler(struct k_timer *timer)
{
	k_work_submit(&auto_conn_start_work);
}

K_TIMER_DEFINE(auto_conn_start_timer, auto_conn_start_timer_handler, NULL);

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	struct ble_device_conn *connection_ptr;
	int err;

	conn_to_mac_addr_str(conn, ble_mac_addr_str);
	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connection_ptr);
	if (err) {
		LOG_ERR("Connection not found for addr %s", ble_mac_addr_str);
	}
	if (conn_err || err) {
		LOG_ERR("Failed to connect to %s (%u)", ble_mac_addr_str,
			conn_err);
		if (connection_ptr) {
			ble_conn_set_connected(connection_ptr, false);
		}
		return;
	}

	if (connection_ptr && connection_ptr->hidden) {
		LOG_DBG("suppressing device_connect");
	} else {
		k_mutex_lock(&output.lock, K_FOREVER);
		device_connect_result_encode(ble_mac_addr_str, true, &output);
		g2c_send(&output.data);
		k_mutex_unlock(&output.lock);
	}

	if (connection_ptr->added_to_allowlist) {
		if (!ble_add_to_allowlist(ble_mac_addr_str, false)) {
			connection_ptr->added_to_allowlist = false;
		}
	}

	/* TODO: set LED pattern indicating BLE device is connected */
	if (!connection_ptr->connected) {
		LOG_INF("Connected: %s", ble_mac_addr_str);
		ble_conn_set_connected(connection_ptr, true);
		ble_subscribe_device(conn, true);
		if (!connection_ptr->hidden) {
			set_shadow_ble_conn(ble_mac_addr_str, false, true);
		}
	} else {
		LOG_INF("Reconnected: %s", ble_mac_addr_str);
	}

	/* Start the timer to begin scanning again. */
	k_timer_start(&auto_conn_start_timer, K_SECONDS(3), K_SECONDS(0));
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];
	struct ble_device_conn *connection_ptr = NULL;

	conn_to_mac_addr_str(conn, ble_mac_addr_str);
	ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connection_ptr);

	if (!ble_conn_mgr_is_desired(ble_mac_addr_str)) {
		LOG_INF("suppressing device_disconnect");
	} else {
		k_mutex_lock(&output.lock, K_FOREVER);
		device_disconnect_result_encode(ble_mac_addr_str, false, &output);
		g2c_send(&output.data);
		k_mutex_unlock(&output.lock);
	}

	/* if device disconnected on purpose, don't bother updating
	 * shadow; it will likely reconnect shortly
	 */
	if (reason != BT_HCI_ERR_REMOTE_USER_TERM_CONN) {
		LOG_INF("Disconnected: %s (reason 0x%02x)", ble_mac_addr_str,
			reason);
		if (connection_ptr && !connection_ptr->hidden) {
			(void)set_shadow_ble_conn(ble_mac_addr_str, false, false);
		}
		if (connection_ptr) {
			ble_conn_set_connected(connection_ptr, false);
		}
	} else {
		LOG_INF("Disconnected: temporary");
	}

	if (connection_ptr &&
	    !connection_ptr->free &&
	    !connection_ptr->added_to_allowlist) {
		if (!ble_add_to_allowlist(connection_ptr->ble_mac_addr_str, true)) {
			connection_ptr->added_to_allowlist = true;
		}
	}

	/* TODO: set BLE device disconnected LED pattern */

	/* Start the timer to begin scanning again. */
	k_timer_start(&auto_conn_start_timer, K_SECONDS(3), K_SECONDS(0));
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	int len = MIN(data->data_len, NAME_LEN - 1);

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void ble_device_found_enc_handler(struct k_work *work)
{
	LOG_DBG("Encoding scan...");
	k_mutex_lock(&output.lock, K_FOREVER);
	device_found_encode(num_devices_found, &output);
	LOG_DBG("Sending scan...");
	g2c_send(&output.data);
	k_mutex_unlock(&output.lock);
}

K_WORK_DEFINE(ble_device_encode_work, ble_device_found_enc_handler);

static void device_found(const bt_addr_le_t *bt_addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad)
{
	char name[NAME_LEN];
	struct ble_scanned_dev *scanned = &ble_scanned_devices[num_devices_found];
	char ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN + 1];

	if (num_devices_found >= MAX_SCAN_RESULTS) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		return;
	}

	le_addr_to_mac_addr_str(bt_addr, ble_mac_addr_str);

	/* Check for duplicate addresses before adding to results. */
	for (int j = 0; j < num_devices_found; j++) {
		if (!(strncasecmp(ble_mac_addr_str,
				  ble_scanned_devices[j].ble_mac_addr_str,
				  BT_ADDR_LE_STR_LEN))) {
			return; /* no need to continue; we saw this */
		}
	}
	/* Update scan results to include this new device's MAC address. */
	memcpy(scanned->ble_mac_addr_str, ble_mac_addr_str, BT_ADDR_LE_DEVICE_LEN);
	scanned->ble_mac_addr_str[BT_ADDR_LE_DEVICE_LEN] = '\0';

	/* Add the device name, if known */
	memset(name, 0, sizeof(name));
	bt_data_parse(ad, data_cb, name);
	strcpy(scanned->name, name);
	if (strlen(name)) {
		num_names_found++;
	}

	scanned->rssi = (int)rssi;
	num_devices_found++;

	LOG_INF("%d. %s, %d, %s", num_devices_found, scanned->ble_mac_addr_str,
		rssi, scanned->name);
}

struct ble_scanned_dev *get_scanned_device(unsigned int i)
{
	if (i < num_devices_found) {
		return &ble_scanned_devices[i];
	} else {
		return NULL;
	}
}

int get_num_scan_results(void)
{
	return num_devices_found;
}

int get_num_scan_names(void)
{
	return num_names_found;
}

static void scan_off_handler(struct k_work *work)
{
	int err;

	LOG_INF("Stopping scan...");

	err = bt_le_scan_stop();
	if (err) {
		LOG_INF("Stopping scanning failed (err %d)", err);
	} else {
		LOG_INF("Scan successfully stopped");
	}

	/* Start the timer to begin scanning again. */
	k_timer_start(&auto_conn_start_timer, K_SECONDS(3), K_SECONDS(0));

	LOG_DBG("Submitting scan...");
	k_work_submit(&ble_device_encode_work);

	if (print_scan_results) {
		print_scan_results = false;

		struct ble_scanned_dev *scanned;
		int i;

		printk("Scan results:\n");
		for (i = 0, scanned = ble_scanned_devices;
		     i < num_devices_found; i++, scanned++) {
			printk("%d. %s, %d, %s\n", i, scanned->ble_mac_addr_str,
			       (int)scanned->rssi, scanned->name);
			k_sleep(K_MSEC(50));
		}
	}
}

K_WORK_DEFINE(scan_off_work, scan_off_handler);

static void scan_timer_handler(struct k_timer *timer)
{
	k_work_submit(&scan_off_work);
}

K_TIMER_DEFINE(scan_timer, scan_timer_handler, NULL);

int ble_add_to_allowlist(const char *addr_str, bool add)
{
	int err;
	bt_addr_le_t bt_addr;

	LOG_INF("%s allowlist: %s", add ? "Adding address to" : "Removing address from",
		addr_str);

	bt_conn_create_auto_stop();

	err = bt_addr_le_from_str(addr_str, "random", &bt_addr);
	if (err) {
		LOG_ERR("Invalid peer address: %d", err);
		goto done;
	}

	err = add ? bt_le_filter_accept_list_add(&bt_addr) :
		    bt_le_filter_accept_list_remove(&bt_addr);
	if (err) {
		LOG_ERR("Error %s allowlist: %d", add ? "adding to" : "removing from", err);
	}

done:
	/* Start the timer to begin scanning again. */
	k_timer_start(&auto_conn_start_timer, K_SECONDS(3), K_SECONDS(0));

	return err;
}

void ble_stop_activity(void)
{
	int err;

	LOG_INF("Stop scan...");
	err = bt_le_scan_stop();
	if (err) {
		LOG_DBG("Error stopping scan: %d", err);
	}

	LOG_INF("Stop autoconnect...");
	err = bt_conn_create_auto_stop();
	if (err) {
		LOG_DBG("Error stopping autoconnect: %d", err);
	}

	LOG_INF("Clear allowlist...");
	err = bt_le_filter_accept_list_clear();
	if (err) {
		LOG_DBG("Error clearing allowlist: %d", err);
	}

	struct ble_device_conn *conn;
	uint8_t ret;

	LOG_INF("Disconnect devices...");
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		conn = get_connected_device(i);
		if (conn && (conn->connected)) {
			ret = disconnect_device_by_addr(conn->ble_mac_addr_str);
			if (ret) {
				LOG_DBG("Error disconnecting %s",
					conn->ble_mac_addr_str);
			}
		}
	}
}

int device_discovery_send(struct ble_device_conn *conn_ptr)
{
	if (conn_ptr && conn_ptr->hidden) {
		LOG_DBG("suppressing device_discovery_send");
		return 0;
	}
	k_mutex_lock(&output.lock, K_FOREVER);
	int ret = device_discovery_encode(conn_ptr, &output);

	if (!ret) {
		LOG_INF("Sending discovery; JSON Size: %d", output.data.len);
		g2c_send(&output.data);
	}
	memset((char *)output.data.ptr, 0, output.data.len);
	k_mutex_unlock(&output.lock);

	return ret;
}

void scan_start(bool print)
{
	int err;

	print_scan_results = print;
	num_devices_found = 0;
	num_names_found = 0;
	memset(ble_scanned_devices, 0, sizeof(ble_scanned_devices));

	struct bt_le_scan_param param = {
		.type     = BT_LE_SCAN_TYPE_ACTIVE,
		.options  = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = 0x0010,
		.window   = 0x0010,
	};

	if (!discover_in_progress) {
		/* Stop the auto connect */
		bt_conn_create_auto_stop();

		err = bt_le_scan_start(&param, device_found);
		if (err) {
			LOG_ERR("Bluetooth set active scan failed "
				"(err %d)\n", err);
		} else {
			LOG_INF("Bluetooth active scan enabled");

			/* TODO: Get scan timeout from scan message */
			k_timer_start(&scan_timer, K_SECONDS(10), K_SECONDS(0));
		}

		scan_waiting = false;
	} else {
		LOG_INF("Scan waiting... Discover in progress");
		scan_waiting = true;
	}
}

static void ble_ready(int err)
{
	LOG_INF("Bluetooth ready");

	bt_conn_cb_register(&conn_callbacks);
}

int ble_init(void)
{
	int err;

	LOG_INF("Initializing Bluetooth..");
	k_mutex_init(&output.lock);

	err = bt_enable(ble_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	for (int i = 0; i < MAX_SCAN_RESULTS; i++) {
		memset(ble_scanned_devices[i].name, 0,
		       sizeof(ble_scanned_devices[1].name));
	}

	return 0;
}

#if defined(CONFIG_BOARD_NRF9160DK_NRF9160_NS)
/* This function is missing in the Zephyr file
 * boards/nordic/nrf9160dk/nrf52840_reset.c. It is now
 * required by the Bluetooth stack.
 */
int bt_h4_vnd_setup(const struct device *dev)
{
	return 0;
}
#endif
