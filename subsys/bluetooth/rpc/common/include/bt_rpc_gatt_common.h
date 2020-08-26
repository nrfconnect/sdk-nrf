/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef BT_RPC_GATT_COMMON_H_
#define BT_RPC_GATT_COMMON_H_

#include <sys/types.h>

#define BT_RPC_GATT_ATTR_SPECIAL_READ_SYNC ((uintptr_t)1)
#define BT_RPC_GATT_ATTR_SPECIAL_WRITE_SYNC ((uintptr_t)2)

#define BT_RPC_GATT_ATTR_SPECIAL_SERVICE (UINTPTR_MAX - (uintptr_t)1)
#define BT_RPC_GATT_ATTR_SPECIAL_SECONDARY (UINTPTR_MAX - (uintptr_t)2)
#define BT_RPC_GATT_ATTR_SPECIAL_INCLUDED (UINTPTR_MAX - (uintptr_t)3)
#define BT_RPC_GATT_ATTR_SPECIAL_CHRC (UINTPTR_MAX - (uintptr_t)4)
#define BT_RPC_GATT_ATTR_SPECIAL_CCC (UINTPTR_MAX - (uintptr_t)5)
#define BT_RPC_GATT_ATTR_SPECIAL_CEP (UINTPTR_MAX - (uintptr_t)6)
#define BT_RPC_GATT_ATTR_SPECIAL_CUD (UINTPTR_MAX - (uintptr_t)7)
#define BT_RPC_GATT_ATTR_SPECIAL_CPF (UINTPTR_MAX - (uintptr_t)8)
#define BT_RPC_GATT_ATTR_SPECIAL_FIRST (UINTPTR_MAX - (uintptr_t)9)

#define BT_RPC_GATT_ATTR_READ_PRESENT_FLAG 0x100
#define BT_RPC_GATT_ATTR_WRITE_PRESENT_FLAG 0x200

struct bt_gatt_attr;
struct bt_gatt_service_static;

typedef	ssize_t (*bt_gatt_attr_read_cb_t)(struct bt_conn *conn,
					  const struct bt_gatt_attr *attr,
					  void *buf, uint16_t len,
					  uint16_t offset);
typedef ssize_t (*bt_gatt_attr_write_cb_t)(struct bt_conn *conn,
					   const struct bt_gatt_attr *attr,
					   const void *buf, uint16_t len,
					   uint16_t offset, uint8_t flags);

#define BT_RPC_GATT_SPECIAL_FLAGS(_read_sync_conf, _write_sync_conf) \
	((bt_gatt_attr_write_cb_t)( \
		(IS_ENABLED(_read_sync_conf) || IS_ENABLED(BT_RPC_GATT_READ_SYNC) ? BT_RPC_GATT_ATTR_SPECIAL_READ_SYNC : 0) | \
		(IS_ENABLED(_write_sync_conf) || IS_ENABLED(BT_RPC_GATT_WRITE_SYNC) ? BT_RPC_GATT_ATTR_SPECIAL_WRITE_SYNC : 0) \
	))

int bt_rpc_gatt_add_service(const struct bt_gatt_service_static *service);
int bt_rpc_gatt_attr_to_index(struct bt_gatt_attr *attr);
const struct bt_gatt_attr *bt_rpc_gatt_index_to_attr(int index);
int bt_rpc_gatt_srv_attr_to_index(uint8_t service_index, struct bt_gatt_attr *attr);

#endif /* BT_RPC_GATT_COMMON_H_ */
