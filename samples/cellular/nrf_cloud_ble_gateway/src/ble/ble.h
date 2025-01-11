/* Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BLE_H_
#define _BLE_H_

#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#define MAX_SCAN_RESULTS 10
#define BT_ATTR_SERVICE 0
#define BT_ATTR_CHRC 1
#define BT_ATTR_CCC 3
#define BT_ADDR_LE_DEVICE_LEN 17
#define NAME_LEN 30
#define UUID_STR_LEN 37
#define BT_MAX_SUBSCRIBES 25

struct ble_scanned_dev {
	int rssi;
	char name[NAME_LEN];
	char ble_mac_addr_str[BT_ADDR_LE_STR_LEN + 1];
};

struct ble_device_conn;
struct desired_conn;

typedef int (*notification_cb_t)(const char *ble_mac_addr_str, const char *chrc_uuid,
				 uint8_t *data, uint16_t len);

int ble_init(void);
int ble_add_to_allowlist(const char *addr_str, bool add);
void scan_start(bool print_scan);
void ble_register_notify_callback(notification_cb_t callback);
int ble_subscribe(const char *ble_mac_addr_str, const char *chrc_uuid, uint8_t value_type);
int ble_subscribe_handle(const char *ble_mac_addr_str, uint16_t handle, uint8_t value_type);
int ble_subscribe_all(const char *ble_mac_addr_str, uint8_t value_type);
int gatt_read(const char *ble_mac_addr_str, const char *chrc_uuid, bool ccc);
int gatt_write(const char *ble_mac_addr_str, const char *chrc_uuid, uint8_t *data,
	       uint16_t data_len, bt_gatt_write_func_t cb);
int gatt_write_without_response(const char *ble_mac_addr_str, const char *chrc_uuid, uint8_t *data,
				uint16_t data_len);
int ble_discover(struct ble_device_conn *conn_ptr);
void bt_uuid_get_str(const struct bt_uuid *uuid, char *str, size_t len);
void bt_to_upper(char *ble_mac_addr_str, uint8_t addr_len);
int disconnect_device_by_addr(const char *ble_mac_addr_str);
int device_discovery_send(struct ble_device_conn *conn_ptr);
struct ble_scanned_dev *get_scanned_device(unsigned int i);
int get_num_scan_results(void);
int get_num_scan_names(void);
void ble_stop_activity(void);
int set_shadow_desired_conn(const struct desired_conn *desired, int num_desired);
int set_shadow_ble_conn(const char *ble_address, bool connecting, bool connected);

#endif /* _BLE_H_ */
