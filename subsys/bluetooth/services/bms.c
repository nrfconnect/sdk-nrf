/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <bluetooth/services/bms.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bms, CONFIG_BT_BMS_LOG_LEVEL);

#define UINT24_SIZE	3

#define BT_ATT_ERR_OP_CODE_NOT_SUPPORTED	0x80
#define BT_ATT_ERR_OPERATION_FAILED		0x81

/* Features relevant for LE Transport only */
enum bt_bms_le_feature {
	/* Delete bond of the requesting device */
	BT_BMS_REQ_DEV = BIT(4),

	/* Delete bond of the requesting device with an authorization code */
	BT_BMS_REQ_DEV_AUTH_CODE = BIT(5),

	/* Delete all bonds on the device */
	BT_BMS_ALL_BONDS = BIT(10),

	/* Delete all bonds on the device with an authorization code */
	BT_BMS_ALL_BONDS_AUTH_CODE = BIT(11),

	/* Delete all bonds on the device except for the bond of
	 * the requesting device
	 */
	BT_BMS_ALL_EXCEPT_REQ_DEV = BIT(16),

	/* Delete all bonds on the device except for the bond of
	 * the requesting device with an authorization code
	 */
	BT_BMS_ALL_EXCEPT_REQ_DEV_AUTH_CODE = BIT(17),
};

/* State of BMS instance */
enum bt_bms_state {
	/* Idle */
	IDLE,

	/* Control point operation in progress */
	OPERATION_IN_PROGRESS,

	/* Bond deletion operation in progress */
	DELETION_IN_PROGRESS,
};

/* Execution descriptor */
struct bt_bms_bond_add_exec {
	/* Kernel work item */
	struct k_work work;

	/* Bond operation code */
	enum bt_bms_op op_code;

	/* Requestor address */
	bt_addr_le_t req_addr;
};

static struct bt_bms {
	enum bt_bms_state state;
	struct bt_bms_bond_add_exec bond_add_exec;
	struct k_work bond_del_exec;
	const struct bt_bms_cb *cbs;
	struct bt_bms_features features;
	bt_addr_le_t bonded_addrs[CONFIG_BT_MAX_PAIRED];
} bms_inst;

static inline bool is_bond_item_free(const bt_addr_le_t *addr)
{
	const bt_addr_le_t free_addr = {0};

	return (memcmp(&free_addr, addr, sizeof(free_addr)) == 0);
}

static void bond_item_add(const bt_addr_le_t *addr)
{
	int32_t free_item_index = -1;

	for (uint32_t i = 0; i < ARRAY_SIZE(bms_inst.bonded_addrs); i++) {
		if (bt_addr_le_cmp(addr, &bms_inst.bonded_addrs[i]) == 0) {
			LOG_DBG("This bonded address is already tracked");
			return;
		}

		if (free_item_index == -1) {
			if (is_bond_item_free(&bms_inst.bonded_addrs[i])) {
				free_item_index = (int32_t) i;
			}
		}
	}

	if (free_item_index == -1) {
		LOG_ERR("No space left to track a new bonded address");
		return;
	}

	memcpy(&bms_inst.bonded_addrs[free_item_index], addr, sizeof(*addr));
}

static void bond_item_remove(bt_addr_le_t *addr)
{
	int err;
	struct bt_conn *conn;
	char addr_str[BT_ADDR_LE_STR_LEN];

	if (is_bond_item_free(addr)) {
		return;
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, addr);
	if (conn) {
		LOG_DBG("Bond information for: %s will be removed on "
			"disconnect", addr_str);

		bt_conn_unref(conn);
	} else {
		LOG_INF("Removing bond information for: %s", addr_str);

		err = bt_unpair(BT_ID_DEFAULT, addr);
		if (err) {
			LOG_ERR("Unable to remove bond information: %d", err);
		}

		memset(addr, 0, sizeof(*addr));
	}
}

static void bond_items_remove(void)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(bms_inst.bonded_addrs); i++) {
		bond_item_remove(&bms_inst.bonded_addrs[i]);
	}
}

static void all_bond_items_add(const struct bt_bond_info *info,
			       void *user_data)
{
	bond_item_add(&info->addr);
}

static void rest_bond_items_add(const struct bt_bond_info *info,
				void *user_data)
{
	const bt_addr_le_t *req_addr = (const bt_addr_le_t *) user_data;

	if (bt_addr_le_cmp(&info->addr, req_addr) != 0) {
		bond_item_add(&info->addr);
	}
}

static void bond_add_execute(struct k_work *item)
{
	struct bt_bms_bond_add_exec *exec =
		CONTAINER_OF(item, struct bt_bms_bond_add_exec, work);

	/* Add bonds to the storage on client's delete request. */
	switch (exec->op_code) {
	case BT_BMS_OP_DEL_REQ_BOND:
		bond_item_add(&exec->req_addr);
		break;
	case BT_BMS_OP_DEL_ALL_BONDS:
		bt_foreach_bond(BT_ID_DEFAULT, all_bond_items_add, NULL);
		break;
	case BT_BMS_OP_DEL_REST_BONDS:
		bt_foreach_bond(BT_ID_DEFAULT, rest_bond_items_add,
				&exec->req_addr);
		break;
	}

	/* Transition to the next state and delete bonds from disconnected
	 * peers using a different work item.
	 */
	bms_inst.state = DELETION_IN_PROGRESS;
	k_work_submit(&bms_inst.bond_del_exec);

	LOG_DBG("Bond adding work is done");
}

static void bond_del_execute(struct k_work *item)
{
	bond_items_remove();
	bms_inst.state = IDLE;

	LOG_DBG("Bond deleting work is done");
}

static bool ctrl_pt_op_code_validate(enum bt_bms_op op_code)
{
	switch (op_code) {
	case BT_BMS_OP_DEL_REQ_BOND:
		return bms_inst.features.delete_requesting.supported;
	case BT_BMS_OP_DEL_ALL_BONDS:
		return bms_inst.features.delete_all.supported;
	case BT_BMS_OP_DEL_REST_BONDS:
		return bms_inst.features.delete_rest.supported;
	default:
		return false;
	}
}

static bool ctrl_pt_auth_op_check(enum bt_bms_op op_code)
{
	switch (op_code) {
	case BT_BMS_OP_DEL_REQ_BOND:
		return bms_inst.features.delete_requesting.authorize;
	case BT_BMS_OP_DEL_ALL_BONDS:
		return bms_inst.features.delete_all.authorize;
	case BT_BMS_OP_DEL_REST_BONDS:
		return bms_inst.features.delete_rest.authorize;
	default:
		return false;
	}
}

static bool ctrl_pt_authorize(struct bt_conn *conn,
			      struct bt_bms_authorize_params *params)
{
	if (!ctrl_pt_auth_op_check(params->op_code)) {
		return true;
	}

	if (bms_inst.cbs->authorize) {
		return bms_inst.cbs->authorize(conn, params);
	}

	LOG_ERR("Authorization callback is expected for this BMS "
		"operation");
	return false;
}

static ssize_t ctrl_pt_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle,
		(void *)conn);

	uint8_t *cp_proc;
	enum bt_bms_op op_code;
	struct bt_bms_authorize_params params;

	if (offset != 0) {
		LOG_ERR("Invalid offset of Write Request");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (bms_inst.state != IDLE) {
		LOG_ERR("Server is busy");
		return BT_GATT_ERR(BT_ATT_ERR_OPERATION_FAILED);
	}

	cp_proc = (uint8_t *) buf;
	op_code = (enum bt_bms_op) cp_proc[0];
	if (!ctrl_pt_op_code_validate(op_code)) {
		LOG_ERR("Operation code not supported 0x%02X", op_code);
		return BT_GATT_ERR(BT_ATT_ERR_OP_CODE_NOT_SUPPORTED);
	}

	params.op_code  = op_code;
	params.code     = cp_proc + sizeof(op_code);
	params.code_len = len - sizeof(op_code);
	if (!ctrl_pt_authorize(conn, &params)) {
		LOG_ERR("Authorization failed");
		return BT_GATT_ERR(BT_ATT_ERR_AUTHORIZATION);
	}

	bms_inst.state = OPERATION_IN_PROGRESS;

	bms_inst.bond_add_exec.op_code = op_code;
	memcpy(&bms_inst.bond_add_exec.req_addr,
	       bt_conn_get_dst(conn),
	       sizeof(bms_inst.bond_add_exec.req_addr));
	k_work_submit(&bms_inst.bond_add_exec.work);

	return len;
}

static inline uint32_t feature_encode(void)
{
	uint32_t feature = 0;

	if (bms_inst.features.delete_requesting.supported) {
		if (bms_inst.features.delete_requesting.authorize) {
			feature |= BT_BMS_REQ_DEV_AUTH_CODE;
		} else {
			feature |= BT_BMS_REQ_DEV;
		}
	}

	if (bms_inst.features.delete_all.supported) {
		if (bms_inst.features.delete_all.authorize) {
			feature |= BT_BMS_ALL_BONDS_AUTH_CODE;
		} else {
			feature |= BT_BMS_ALL_BONDS;
		}
	}

	if (bms_inst.features.delete_rest.supported) {
		if (bms_inst.features.delete_rest.authorize) {
			feature |= BT_BMS_ALL_EXCEPT_REQ_DEV_AUTH_CODE;
		} else {
			feature |= BT_BMS_ALL_EXCEPT_REQ_DEV;
		}
	}

	return feature;
}

static inline uint16_t feature_size_get(uint32_t feature)
{
	if (feature <= UINT8_MAX) {
		return sizeof(uint8_t);
	} else if (feature <= UINT16_MAX) {
		return sizeof(uint16_t);
	} else {
		return UINT24_SIZE;
	}
}

static ssize_t feature_read(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   void *buf,
			   uint16_t len,
			   uint16_t offset)
{
	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle,
		(void *)conn);

	uint32_t feature = feature_encode();
	uint16_t feature_size = feature_size_get(feature);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &feature,
				 feature_size);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_DBG("Disconnected (reason %u)", reason);

	if (bms_inst.state != OPERATION_IN_PROGRESS) {
		bms_inst.state = DELETION_IN_PROGRESS;
		k_work_submit(&bms_inst.bond_del_exec);
	}
}

static struct bt_conn_cb bms_conn_callbacks = {
	.disconnected     = disconnected,
};

/* Bond Management Service Declaration */
BT_GATT_SERVICE_DEFINE(bms_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_BMS),
	BT_GATT_CHARACTERISTIC(BT_UUID_BMS_CONTROL_POINT,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE_AUTHEN,
			       NULL, ctrl_pt_write,
			       NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_BMS_FEATURE,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_AUTHEN,
			       feature_read, NULL,
			       NULL),
);

int bt_bms_init(const struct bt_bms_init_params *init_params)
{
	if (memcmp(&init_params->features,
		   (struct bt_bms_features[]) {0},
		   sizeof(init_params->features)) == 0) {
		return -EINVAL;
	}

	if ((!init_params->cbs ||
	     !init_params->cbs->authorize) &&
	    (init_params->features.delete_all.authorize ||
	     init_params->features.delete_requesting.authorize ||
	     init_params->features.delete_rest.authorize)) {
		return -EINVAL;
	}

	bms_inst.cbs = init_params->cbs;
	bms_inst.features = init_params->features;

	k_work_init(&bms_inst.bond_add_exec.work, bond_add_execute);
	k_work_init(&bms_inst.bond_del_exec, bond_del_execute);

	bt_conn_cb_register(&bms_conn_callbacks);

	memset(bms_inst.bonded_addrs, 0, sizeof(bms_inst.bonded_addrs));
	bms_inst.state = IDLE;

	LOG_DBG("BMS initialization successful");

	return 0;
}
