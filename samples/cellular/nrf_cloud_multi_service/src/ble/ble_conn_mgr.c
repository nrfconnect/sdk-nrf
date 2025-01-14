/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <nrf_cloud_fota.h>

#include "gateway.h"
#include "ble_conn_mgr.h"
#include "ble_codec.h"
#include "cJSON.h"
#include "ble.h"
#include "cloud_connection.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ble_conn_mgr, CONFIG_NRFCLOUD_BLE_GATEWAY_LOG_LEVEL);

static int num_connected;
static struct ble_device_conn connected_ble_devices[CONFIG_BT_MAX_CONN];

static struct desired_conn desired_connections[CONFIG_BT_MAX_CONN];

#define CONN_MGR_STACK_SIZE 4096
#define CONN_MGR_PRIORITY 1

static struct uuid_handle_pair *find_pair_by_handle(uint16_t handle,
						    const struct ble_device_conn *conn_ptr,
						    int *index);
static int ble_conn_mgr_get_free_conn(struct ble_device_conn **conn_ptr);

static void process_connection(int i)
{
	int err;
	struct ble_device_conn *dev = &connected_ble_devices[i];

	if (dev->free) {
		return;
	}

	if (!await_cloud_ready(K_MSEC(50))) {
		return;
	}

	/* Add devices to allowlist */
	if (!dev->added_to_allowlist && !dev->shadow_updated && !dev->connected) {
		if (!dev->hidden) {
			err = set_shadow_ble_conn(dev->ble_mac_addr_str, true, false);
			if (!err) {
				dev->shadow_updated = true;
				LOG_INF("Shadow updated.");
			}
		}
		if (!ble_add_to_allowlist(dev->ble_mac_addr_str, true)) {
			dev->added_to_allowlist = true;
			LOG_INF("Device added to allowlist.");
		}
	}

	/* Connected. Do discovering if not discovered or currently
	 * discovering.
	 */
	if (dev->connected && !dev->discovered && !dev->discovering) {

		err = ble_discover(dev);

		if (!err) {
			LOG_DBG("ble_discover(%s) failed: %d",
				dev->ble_mac_addr_str, err);
		}
	}

	/* Discovering done. Encode and send. */
	if (dev->connected && dev->encode_discovered) {
		dev->encode_discovered = false;
		device_discovery_send(&connected_ble_devices[i]);

		bt_addr_t ble_id;

		err = bt_addr_from_str(connected_ble_devices[i].ble_mac_addr_str,
				       &ble_id);
		if (!err) {
#if defined(CONFIG_GATEWAY_BLE_FOTA)
			LOG_INF("Checking for BLE update...");
			nrf_cloud_fota_ble_update_check(&ble_id);
#endif
		}
	}
}

static void connection_manager(int unused1, int unused2, int unused3)
{
	int i;
	bool printed = false;

	ble_conn_mgr_init();
	while (1) {

		for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			/* Manager is busy. Do nothing. */
			if (connected_ble_devices[i].discovering) {
				if (!printed) {
					LOG_DBG("Connection work busy.");
					printed = true;
				}
				goto end;
			}
		}
		printed = false;

		for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
			process_connection(i);
		}

end:
		/* give up the CPU for a while; otherwise we spin much faster
		 * than the cloud side could reasonably change things,
		 * or that devices might connect and disconnect, and so
		 * would needlessly consume CPU
		 */
		k_sleep(K_MSEC(100));
	}
}

K_THREAD_DEFINE(conn_mgr_thread, CONN_MGR_STACK_SIZE,
		connection_manager, NULL, NULL, NULL,
		CONN_MGR_PRIORITY, 0, 0);

static void init_conn(struct ble_device_conn *dev)
{
	memset(dev, 0, sizeof(struct ble_device_conn));
	dev->free = true;
}

static void ble_conn_mgr_conn_reset(struct ble_device_conn
					*dev)
{
	struct uuid_handle_pair *uuid_handle;

	LOG_INF("Connection removed to %s", dev->ble_mac_addr_str);

	if (!dev->free) {
		if (num_connected) {
			num_connected--;
		}
	}

	/* free in backwards order to try to reduce fragmentation */
	while (dev->num_pairs) {
		uuid_handle = dev->uuid_handle_pairs[dev->num_pairs - 1];
		if (uuid_handle != NULL) {
			dev->uuid_handle_pairs[dev->num_pairs - 1] = NULL;
			k_free(uuid_handle);
		}
		dev->num_pairs--;
	}
	init_conn(dev);
}

void ble_conn_mgr_update_connections(void)
{
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct ble_device_conn *dev = &connected_ble_devices[i];

		if (dev->connected || dev->added_to_allowlist) {
			dev->disconnect = true;

			for (int j = 0; j < CONFIG_BT_MAX_CONN; j++) {
				if (desired_connections[j].active) {
					if (!strcmp(desired_connections[j].ble_mac_addr_str,
						    dev->ble_mac_addr_str)) {
						/* If in the list then don't
						 * disconnect.
						 */
						dev->disconnect = false;
						break;
					}
				}
			}

			if (dev->disconnect) {
				int err;

				LOG_INF("Cloud: disconnect device %s",
					dev->ble_mac_addr_str);
				if (dev->added_to_allowlist) {
					if (!ble_add_to_allowlist(dev->ble_mac_addr_str, false)) {
						dev->added_to_allowlist = false;
					}
				}
				err = disconnect_device_by_addr(dev->ble_mac_addr_str);
				if (err) {
					LOG_ERR("Device might still be connected: %d",
						err);
				}
				ble_conn_mgr_conn_reset(dev);
				if (IS_ENABLED(CONFIG_SETTINGS)) {
					LOG_INF("Saving settings");
					settings_save();
				}
			}
		}
	}
}

static void ble_conn_mgr_change_desired(const char *ble_mac_addr_str, uint8_t index,
				 bool active, bool manual)
{
	if (index <= CONFIG_BT_MAX_CONN) {
		strncpy(desired_connections[index].ble_mac_addr_str, ble_mac_addr_str,
			DEVICE_ADDR_LEN);
		desired_connections[index].active = active;
		desired_connections[index].manual = manual;

		LOG_INF("Desired Connection %s: %s %s",
			active ? "Added" : "Removed",
			ble_mac_addr_str,
			manual ? "(manual)" : "");
	}
}

struct desired_conn *get_desired_array(int *array_size)
{
	if (array_size == NULL) {
		return NULL;
	}
	*array_size = ARRAY_SIZE(desired_connections);
	return desired_connections;
}

static int find_desired_connection(const char *ble_mac_addr_str,
				   struct desired_conn **pcon)
{
	struct desired_conn *con;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		con = &desired_connections[i];
		if ((strncmp(ble_mac_addr_str, con->ble_mac_addr_str,
			     sizeof(con->ble_mac_addr_str)) == 0) &&
		    (ble_mac_addr_str[0] != '\0')) {
			if (pcon) {
				*pcon = con;
			}
			return i;
		}
	}
	if (pcon) {
		*pcon = NULL;
	}
	return -EINVAL;
}

void ble_conn_mgr_update_desired(const char *ble_mac_addr_str, uint8_t index)
{
	ble_conn_mgr_change_desired(ble_mac_addr_str, index, true, false);
}

int ble_conn_mgr_add_desired(const char *ble_mac_addr_str, bool manual)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!desired_connections[i].active) {
			ble_conn_mgr_change_desired(ble_mac_addr_str, i, true, manual);
			return 0;
		}
	}
	return -EINVAL;
}

int ble_conn_mgr_rem_desired(const char *ble_mac_addr_str, bool manual)
{
	if (ble_mac_addr_str == NULL) {
		return -EINVAL;
	}

	int i = find_desired_connection(ble_mac_addr_str, NULL);

	if (i >= 0) {
		ble_conn_mgr_change_desired(ble_mac_addr_str, i, false, manual);
		return 0;
	}
	return -EINVAL;
}

void ble_conn_mgr_clear_desired(bool all)
{
	LOG_INF("Desired Connections Cleared.");

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!desired_connections[i].manual || all) {
			desired_connections[i].active = false;
		}
	}
}

bool ble_conn_mgr_enabled(const char *ble_mac_addr_str)
{
	struct desired_conn *con;

	if (ble_mac_addr_str == NULL) {
		return false;
	}

	find_desired_connection(ble_mac_addr_str, &con);

	if (con != NULL) {
		return !con->manual;
	}

	return true;
}

bool ble_conn_mgr_is_desired(const char *ble_mac_addr_str)
{
	struct desired_conn *con;

	if (ble_mac_addr_str == NULL) {
		return false;
	}

	find_desired_connection(ble_mac_addr_str, &con);

	return (con != NULL) && (con->active);
}

int ble_conn_mgr_generate_path(const struct ble_device_conn *conn_ptr,
			       uint16_t handle, char *path, bool ccc)
{
	char path_str[BT_MAX_PATH_LEN];
	char service_uuid[BT_UUID_STR_LEN] = {0};
	char ccc_uuid[BT_UUID_STR_LEN];
	char chrc_uuid[BT_UUID_STR_LEN];
	uint8_t path_depth = 0;
	struct uuid_handle_pair *uuid_handle;
	int i;

	path_str[0] = '\0';
	path[0] = '\0';


	uuid_handle = find_pair_by_handle(handle, conn_ptr, &i);
	if (uuid_handle == NULL) {
		LOG_ERR("Path not generated; handle %u not found for ble_mac_addr_str %s",
			handle, conn_ptr->ble_mac_addr_str);
		return -ENXIO;
	}

	path_depth = uuid_handle->path_depth;

	get_uuid_str(uuid_handle, chrc_uuid, BT_UUID_STR_LEN);

	if (ccc && ((i + 1) < conn_ptr->num_pairs)) {
		uuid_handle = conn_ptr->uuid_handle_pairs[i + 1];
		if (uuid_handle == NULL) {
			LOG_ERR("Path not generated; handle after %u "
				"not found for ble_addr %s",
				handle, conn_ptr->ble_mac_addr_str);
			return -ENXIO;
		}
		get_uuid_str(uuid_handle, ccc_uuid, BT_UUID_STR_LEN);
	} else {
		ccc_uuid[0] = '\0';
	}

	for (int j = i; j >= 0; j--) {
		uuid_handle = conn_ptr->uuid_handle_pairs[j];
		if (uuid_handle == NULL) {
			continue;
		}
		if (uuid_handle->is_service) {
			get_uuid_str(uuid_handle, service_uuid,
				     BT_UUID_STR_LEN);
			break;
		}
	}

	if (!ccc) {
		snprintk(path_str, BT_MAX_PATH_LEN, "%s/%s",
			 service_uuid, chrc_uuid);
	} else {
		snprintk(path_str, BT_MAX_PATH_LEN, "%s/%s/%s",
			 service_uuid, chrc_uuid, ccc_uuid);
	}

	bt_to_upper(path_str, strlen(path_str));
	memset(path, 0, BT_MAX_PATH_LEN);
	memcpy(path, path_str, strlen(path_str));
	LOG_DBG("Num pairs: %d, CCC: %d, depth: %d, generated path: %s",
		conn_ptr->num_pairs, ccc, path_depth, path_str);
	return 0;
}

int ble_conn_mgr_add_conn(const char *ble_mac_addr_str)
{
	int err = 0;
	struct ble_device_conn *connected_ble_ptr;

	/* Check if already added */
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connected_ble_devices[i].free == false) {
			if (!strcmp(ble_mac_addr_str, connected_ble_devices[i].ble_mac_addr_str)) {
				LOG_DBG("Connection already exsists");
				return 1;
			}
		}
	}

	err = ble_conn_mgr_get_free_conn(&connected_ble_ptr);

	if (err) {
		LOG_ERR("No free connections");
		return err;
	}

	memcpy(connected_ble_ptr->ble_mac_addr_str, ble_mac_addr_str, DEVICE_ADDR_LEN);
	connected_ble_ptr->free = false;
	err = bt_addr_le_from_str(ble_mac_addr_str, "random", &connected_ble_ptr->bt_addr);
	if (err) {
		LOG_ERR("Address from string failed (err %d)", err);
	}
	num_connected++;
	LOG_INF("BLE conn to %s added to manager", ble_mac_addr_str);
	return err;
}

int ble_conn_set_connected(struct ble_device_conn *connected_ble_ptr, bool connected)
{
	int err = 0;

	if (connected) {
		connected_ble_ptr->connected = true;
	} else {
		connected_ble_ptr->connected = false;
		connected_ble_ptr->shadow_updated = false;
	}
	LOG_INF("Conn updated: connected=%u", connected);
	return err;
}

int ble_conn_mgr_rediscover(const char *ble_mac_addr_str)
{
	int err = 0;
	struct ble_device_conn *connected_ble_ptr;

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ble_ptr);
	if (err) {
		LOG_ERR("Connection not found for ble_mac_addr_str %s", ble_mac_addr_str);
		return err;
	}

	if (!connected_ble_ptr->discovering) {
		if (connected_ble_ptr->discovered) {
			/* cloud wants data again; just send it */
			LOG_INF("Skipping device discovery on %s",
				connected_ble_ptr->ble_mac_addr_str);
			if (connected_ble_ptr->connected) {
				connected_ble_ptr->encode_discovered = true;
			} else {
				err = device_discovery_send(connected_ble_ptr);
			}
		} else {
			connected_ble_ptr->num_pairs = 0;
		}
	}

	return err;
}

/* an nRF5 SDK device's normal MAC address least significant byte is
 * incremented for the DFU bootloader MAC address
 */
int ble_conn_mgr_calc_other_addr(const char *old_addr, char *new_addr, int len, bool normal)
{
	bt_addr_le_t btaddr;
	int err;

	err = bt_addr_le_from_str(old_addr, "random", &btaddr);
	if (err) {
		LOG_ERR("Could not convert address");
		return err;
	}
	if (normal) {
		btaddr.a.val[0]--;
	} else {
		btaddr.a.val[0]++;
	}

	memset(new_addr, 0, len);
	bt_addr_le_to_str(&btaddr, new_addr, len);
	bt_to_upper(new_addr, len);
	return 0;
}

int ble_conn_mgr_force_dfu_rediscover(const char *ble_mac_addr_str)
{
	int err = 0;
	struct ble_device_conn *connected_ble_ptr;

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ble_ptr);
	if (err) {
		LOG_ERR("Connection not found for ble_mac_addr_str %s", ble_mac_addr_str);
		return err;
	}

	if (!connected_ble_ptr->discovering) {
		LOG_INF("Marking device %s to be rediscovered",
			ble_mac_addr_str);
		connected_ble_ptr->discovered = false;
		connected_ble_ptr->num_pairs = 0;
	}

	return 0;
}

int ble_conn_mgr_remove_conn(const char *ble_mac_addr_str)
{
	int err = 0;
	struct ble_device_conn *connected_ble_ptr;

	err = ble_conn_mgr_get_conn_by_addr(ble_mac_addr_str, &connected_ble_ptr);
	if (err) {
		LOG_ERR("Connection not found for ble_mac_addr_str %s", ble_mac_addr_str);
		return err;
	}

	ble_conn_mgr_conn_reset(connected_ble_ptr);
	return err;
}


static int ble_conn_mgr_get_free_conn(struct ble_device_conn **conn_ptr)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connected_ble_devices[i].free == true) {
			*conn_ptr = &connected_ble_devices[i];
			LOG_DBG("Found Free connection: %d", i);
			return 0;
		}
	}
	return -ENOMEM;
}

void ble_conn_mgr_check_pending(void)
{
	struct ble_device_conn *conn_ptr = connected_ble_devices;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++, conn_ptr++) {
		if (conn_ptr->connected && conn_ptr->dfu_pending) {
			if (conn_ptr->dfu_attempts) {
				conn_ptr->dfu_attempts--;
			} else {
				conn_ptr->dfu_pending = false;
				continue;
			}
			LOG_INF("Requesting pending DFU job for %s",
				conn_ptr->ble_mac_addr_str);
			nrf_cloud_fota_ble_update_check(&conn_ptr->bt_addr.a);
			break;
		}
	}
}

int ble_conn_mgr_get_conn_by_addr(const char *ble_mac_addr_str, struct ble_device_conn **conn_ptr)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!strcmp(ble_mac_addr_str, connected_ble_devices[i].ble_mac_addr_str)) {
			*conn_ptr = &connected_ble_devices[i];
			LOG_DBG("Conn Found");
			return 0;
		}
	}
	return -ENOENT;

}

bool ble_conn_mgr_is_addr_connected(const char *ble_mac_addr_str)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!strcmp(ble_mac_addr_str, connected_ble_devices[i].ble_mac_addr_str)) {
			return connected_ble_devices[i].connected;
		}
	}
	return false;
}

static struct uuid_handle_pair *find_pair_by_handle(uint16_t handle,
						    const struct ble_device_conn *conn_ptr,
						    int *index)
{
	struct uuid_handle_pair *uuid_handle;

	for (int i = 0; i < conn_ptr->num_pairs; i++) {
		uuid_handle = conn_ptr->uuid_handle_pairs[i];
		if (uuid_handle == NULL) {
			continue;
		}
		if (handle == uuid_handle->handle) {
			if (index != NULL) {
				*index = i;
			}
			return uuid_handle;
		}
	}
	return NULL;
}

static void update_ccc_value(uint16_t handle, const struct ble_device_conn *conn_ptr, uint8_t value)
{
	struct uuid_handle_pair *ccc_handle;

	ccc_handle = find_pair_by_handle(handle + 1, conn_ptr, NULL);
	if (ccc_handle && (ccc_handle->attr_type == BT_ATTR_CCC)) {
		ccc_handle->value[0] = value;
		ccc_handle->value_len = 2;
		LOG_DBG("Updated CCC value");
	} else {
		LOG_DBG("CCC not found for handle: %d", handle);
	}
}

int ble_conn_mgr_set_subscribed(uint16_t handle, uint8_t sub_index,
				const struct ble_device_conn *conn_ptr)
{
	struct uuid_handle_pair *uuid_handle;

	uuid_handle = find_pair_by_handle(handle, conn_ptr, NULL);
	if (uuid_handle) {
		uuid_handle->sub_enabled = true;
		uuid_handle->sub_index = sub_index;
		update_ccc_value(handle, conn_ptr, BT_GATT_CCC_NOTIFY);
		return 0;
	}

	return 1;
}


int ble_conn_mgr_remove_subscribed(uint16_t handle,
				   const struct ble_device_conn *conn_ptr)
{
	struct uuid_handle_pair *uuid_handle;

	uuid_handle = find_pair_by_handle(handle, conn_ptr, NULL);
	if (uuid_handle) {
		uuid_handle->sub_enabled = false;
		update_ccc_value(handle, conn_ptr, 0);
		return 0;
	}
	return 1;
}


int ble_conn_mgr_get_subscribed(uint16_t handle,
				const struct ble_device_conn *conn_ptr,
				bool *status, uint8_t *sub_index)
{
	struct uuid_handle_pair *uuid_handle;

	uuid_handle = find_pair_by_handle(handle, conn_ptr, NULL);
	if (uuid_handle) {
		if (status) {
			*status = uuid_handle->sub_enabled;
		}
		if (sub_index) {
			*sub_index = uuid_handle->sub_index;
		}
		return 0;
	}

	return 1;
}

int ble_conn_mgr_get_uuid_by_handle(uint16_t handle, char *uuid,
				    const struct ble_device_conn *conn_ptr)
{
	char uuid_str[BT_UUID_STR_LEN];
	struct uuid_handle_pair *uuid_handle;

	memset(uuid, 0, BT_UUID_STR_LEN);

	uuid_handle = find_pair_by_handle(handle, conn_ptr, NULL);
	if (uuid_handle) {
		get_uuid_str(uuid_handle, uuid_str, BT_UUID_STR_LEN);
		bt_to_upper(uuid_str, strlen(uuid_str));
		memcpy(uuid, uuid_str, strlen(uuid_str));
		LOG_DBG("Found UUID: %s for handle: %d",
			uuid_str,
			handle);
		return 0;
	}

	LOG_ERR("Handle %u on ble_addr %s not found; num pairs: %d", handle,
		conn_ptr->ble_mac_addr_str,
		(int)conn_ptr->num_pairs);
	return 1;
}

struct uuid_handle_pair *ble_conn_mgr_get_uhp_by_uuid(const char *uuid,
						      const struct ble_device_conn *conn_ptr)
{
	char str[BT_UUID_STR_LEN];

	for (int i = 0; i < conn_ptr->num_pairs; i++) {
		struct uuid_handle_pair *uuid_handle = conn_ptr->uuid_handle_pairs[i];

		if (uuid_handle == NULL) {
			continue;
		}

		get_uuid_str(uuid_handle, str, sizeof(str));
		bt_to_upper(str, strlen(str));
		if (!strcmp(uuid, str)) {
			LOG_DBG("Handle: %d, value_len: %d found for UUID: %s",
				uuid_handle->handle, uuid_handle->value_len, uuid);
			return uuid_handle;
		}
	}

	LOG_ERR("Handle not found for UUID: %s", uuid);
	return NULL;
}

int ble_conn_mgr_get_handle_by_uuid(uint16_t *handle, const char *uuid,
				    const struct ble_device_conn *conn_ptr)
{
	char str[BT_UUID_STR_LEN];

	for (int i = 0; i < conn_ptr->num_pairs; i++) {
		struct uuid_handle_pair *uuid_handle =
			conn_ptr->uuid_handle_pairs[i];

		if (uuid_handle == NULL) {
			continue;
		}

		get_uuid_str(uuid_handle, str, sizeof(str));
		bt_to_upper(str, strlen(str));
		if (!strcmp(uuid, str)) {
			*handle = uuid_handle->handle;
			LOG_DBG("Handle: %d found for UUID: %s", *handle, uuid);
			return 0;
		}
	}

	LOG_ERR("Handle not found for UUID: %s", uuid);
	return 1;
}

int ble_conn_mgr_add_uuid_pair(const struct bt_uuid *uuid, uint16_t handle,
			       uint8_t path_depth, uint8_t properties,
			       uint8_t attr_type,
			       struct ble_device_conn *conn_ptr,
			       bool is_service)
{
	char str[BT_UUID_STR_LEN];

	if (!conn_ptr) {
		LOG_ERR("No connection ptr!");
		return -EINVAL;
	}

	if (conn_ptr->num_pairs >= MAX_UUID_PAIRS) {
		LOG_ERR("Max uuid pair limit reached on %s",
			conn_ptr->ble_mac_addr_str);
		return -E2BIG;
	}

	LOG_DBG("Handle: %d", handle);

	if (!uuid) {
		return 0;
	}

	struct uuid_handle_pair *uuid_handle;
	int err = 0;

	uuid_handle = conn_ptr->uuid_handle_pairs[conn_ptr->num_pairs];
	if (uuid_handle != NULL) {
		/* we already discovered this device */
		if (uuid_handle->uuid_type != uuid->type) {
			if (uuid_handle->uuid_type == BT_UUID_TYPE_16) {
				/* likely got larger, so free and reallocate */
				k_free(uuid_handle);
				uuid_handle = NULL;
			}
		}
	}

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		if (uuid_handle == NULL) {
			uuid_handle = (struct uuid_handle_pair *)
				      k_calloc(1, SMALL_UUID_HANDLE_PAIR_SIZE);
			if (uuid_handle == NULL) {
				LOG_ERR("Out of memory error allocating "
					"for handle %u", handle);
				err = -ENOMEM;
				break;
			}
		}
		memcpy(&uuid_handle->uuid_16, BT_UUID_16(uuid),
		       sizeof(struct bt_uuid_16));
		uuid_handle->uuid_type = uuid->type;
		bt_uuid_get_str(&uuid_handle->uuid_16.uuid, str, sizeof(str));

		LOG_DBG("\tChar: 0x%s", str);
		break;
	case BT_UUID_TYPE_128:
		if (uuid_handle == NULL) {
			uuid_handle = (struct uuid_handle_pair *)
				      k_calloc(1, LARGE_UUID_HANDLE_PAIR_SIZE);
			if (uuid_handle == NULL) {
				LOG_ERR("Out of memory error allocating "
					"for handle %u", handle);
				err = -ENOMEM;
				break;
			}
		}
		memcpy(&uuid_handle->uuid_128, BT_UUID_128(uuid),
		       sizeof(struct bt_uuid_128));
		uuid_handle->uuid_type = uuid->type;
		bt_uuid_get_str(&uuid_handle->uuid_128.uuid, str, sizeof(str));

		LOG_DBG("\tChar: 0x%s", str);
		break;
	default:
		err = -EINVAL;
	}

	if (err) {
		conn_ptr->uuid_handle_pairs[conn_ptr->num_pairs] = NULL;
		return err;
	}

	uuid_handle->properties = properties;
	uuid_handle->attr_type = attr_type;
	uuid_handle->path_depth = path_depth;
	uuid_handle->is_service = is_service;
	uuid_handle->handle = handle;
	LOG_DBG("%d. Handle: %d", conn_ptr->num_pairs, handle);

	/* finally, store it in the array of pointers to uuid_handle_pairs */
	conn_ptr->uuid_handle_pairs[conn_ptr->num_pairs] = uuid_handle;

	conn_ptr->num_pairs++;

	return 0;
}

struct ble_device_conn *get_connected_device(unsigned int i)
{
	if (i < CONFIG_BT_MAX_CONN) {
		if (!connected_ble_devices[i].free) {
			return &connected_ble_devices[i];
		}
	}
	return NULL;
}

int get_num_connected(void)
{
	return num_connected;
}

void ble_conn_mgr_init(void)
{
	num_connected = 0;
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		init_conn(&connected_ble_devices[i]);
	}
}
