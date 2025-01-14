/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_CONN_MGR_H_
#define _BLE_CONN_MGR_H_

#include "ble.h"
#include <zephyr/bluetooth/uuid.h>

#define MAX_UUID_PAIRS 68
#define DEVICE_ADDR_LEN 18

#define BT_MAX_VALUE_LEN 32
#define BT_MAX_UUID_LEN 37
#define BT_MAX_PATH_LEN 111

#define MAX_DFU_ATTEMPTS 3

#define SMALL_UUID_HANDLE_PAIR_SIZE (sizeof(struct uuid_handle_pair) - \
				     sizeof(struct bt_uuid_128) + \
				     sizeof(struct bt_uuid_16))
#define LARGE_UUID_HANDLE_PAIR_SIZE (sizeof(struct uuid_handle_pair))

struct uuid_handle_pair {
	uint16_t handle;
	uint8_t uuid_type;
	uint8_t attr_type;
	uint8_t path_depth;
	uint8_t properties;
	bool is_service;
	bool sub_enabled;
	uint8_t sub_index;
	union {
		struct bt_uuid_16 uuid_16;
		struct bt_uuid_128 uuid_128;
	};
	uint16_t value_len;
	uint8_t value[BT_MAX_VALUE_LEN];
};

struct ble_device_conn {
	char ble_mac_addr_str[DEVICE_ADDR_LEN];
	bt_addr_le_t bt_addr;
	struct uuid_handle_pair *uuid_handle_pairs[MAX_UUID_PAIRS];
	uint8_t num_pairs;
	uint8_t dfu_attempts;
	bool connected : 1;
	bool discovering : 1;
	bool free : 1;
	bool discovered : 1;
	bool encode_discovered : 1;
	bool added_to_allowlist : 1;
	bool shadow_updated : 1;
	bool disconnect : 1;
	bool dfu_pending : 1;
	bool hidden : 1;
};

struct desired_conn {
	char ble_mac_addr_str[DEVICE_ADDR_LEN];
	bool active;
	bool manual;
};

int ble_conn_mgr_add_conn(const char *ble_mac_addr_str);
int ble_conn_mgr_calc_other_addr(const char *old_addr, char *new_addr,
				 int len, bool normal);
int ble_conn_mgr_generate_path(const struct ble_device_conn *conn_ptr,
			       uint16_t handle,
			       char *path, bool ccc);
int ble_conn_mgr_remove_conn(const char *ble_mac_addr_str);
int ble_conn_mgr_get_conn_by_addr(const char *ble_mac_addr_str,
				  struct ble_device_conn **conn_ptr);
int ble_conn_mgr_add_uuid_pair(const struct bt_uuid *uuid, uint16_t handle,
			       uint8_t path_depth, uint8_t properties,
			       uint8_t attr_type,
			       struct ble_device_conn *conn_ptr,
			       bool is_service);
int ble_conn_mgr_get_uuid_by_handle(uint16_t handle, char *uuid,
				    const struct ble_device_conn *conn_ptr);
int ble_conn_mgr_get_handle_by_uuid(uint16_t *handle, const char *uuid,
				    const struct ble_device_conn *conn_ptr);
struct uuid_handle_pair *ble_conn_mgr_get_uhp_by_uuid(const char *uuid,
						      const struct ble_device_conn *conn_ptr);
void ble_conn_mgr_init(void);
int ble_conn_set_connected(struct ble_device_conn *conn_ptr, bool connected);
int ble_conn_mgr_set_subscribed(uint16_t handle, uint8_t sub_index,
				const struct ble_device_conn *conn_ptr);
int ble_conn_mgr_remove_subscribed(uint16_t handle,
				   const struct ble_device_conn *conn_ptr);
int ble_conn_mgr_get_subscribed(uint16_t handle,
				const struct ble_device_conn *conn_ptr,
				bool *status, uint8_t *sub_index);
void ble_conn_mgr_update_desired(const char *ble_mac_addr_str, uint8_t index);
int ble_conn_mgr_add_desired(const char *ble_mac_addr_str, bool manual);
int ble_conn_mgr_rem_desired(const char *ble_mac_addr_str, bool manual);
bool ble_conn_mgr_is_desired(const char *ble_mac_addr_str);
void ble_conn_mgr_clear_desired(bool all);
bool ble_conn_mgr_enabled(const char *ble_mac_addr_str);
void ble_conn_mgr_update_connections(void);
int ble_conn_mgr_rediscover(const char *ble_mac_addr_str);
struct ble_device_conn *get_connected_device(unsigned int i);
int get_num_connected(void);
struct desired_conn *get_desired_array(int *array_size);
bool ble_conn_mgr_is_addr_connected(const char *ble_mac_addr_str);
void ble_conn_mgr_print_mem(void);
int ble_conn_mgr_find_related_addr(const char *old_addr, char *new_addr, int len);
int ble_conn_mgr_force_dfu_rediscover(const char *ble_mac_addr_str);
void ble_conn_mgr_check_pending(void);

#endif
