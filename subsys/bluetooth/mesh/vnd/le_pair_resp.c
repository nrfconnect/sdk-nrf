/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/random/rand32.h>

#include <zephyr/bluetooth/conn.h>
#include <bluetooth/mesh/vnd/le_pair_resp.h>
#include "../model_utils.h"
#include "mesh/net.h"
#include "mesh/access.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_le_pair_resp);

#define BT_MESH_LE_PAIR_OP BT_MESH_MODEL_OP_3(0x11, BT_MESH_VENDOR_COMPANY_ID_LE_PAIR_RESP)

#define BT_MESH_LE_PAIR_OP_RESET 0x00
#define BT_MESH_LE_PAIR_OP_STATUS 0x01

#define STATUS_PASSKEY_SET 0x00
#define STATUS_PASSKEY_NOT_SET 0x01

static uint32_t predefined_passkey = BT_PASSKEY_INVALID;

static int handle_reset(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	uint32_t passkey;
	uint8_t status = STATUS_PASSKEY_SET;
	int err;

	if (buf->len != 0) {
		return -EINVAL;
	}

	BT_MESH_MODEL_BUF_DEFINE(rsp, BT_MESH_LE_PAIR_OP, 5);

	if (predefined_passkey != BT_PASSKEY_INVALID) {
		passkey = predefined_passkey;
	} else {
		passkey = sys_rand32_get() % 1000000;
	}

	err = bt_passkey_set(passkey);
	if (err) {
		LOG_ERR("Unable to set passkey (err: %d)", err);
		status = STATUS_PASSKEY_NOT_SET;
	}

	bt_mesh_model_msg_init(&rsp, BT_MESH_LE_PAIR_OP);
	net_buf_simple_add_u8(&rsp, BT_MESH_LE_PAIR_OP_STATUS);
	net_buf_simple_add_u8(&rsp, status);
	if (status == STATUS_PASSKEY_SET) {
		net_buf_simple_add_le24(&rsp, passkey);
	}

	(void)bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);

	return 0;
}

static int handle_op(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	uint8_t op;

	op = net_buf_simple_pull_u8(buf);
	switch (op) {
	case BT_MESH_LE_PAIR_OP_RESET:
		return handle_reset(model, ctx, buf);
	default:
		LOG_WRN("Unknown opcode: %d", op);
	}

	return 0;
}

const struct bt_mesh_model_op _bt_mesh_le_pair_resp_op[] = {
	{ BT_MESH_LE_PAIR_OP, BT_MESH_LEN_MIN(1), handle_op, },
	BT_MESH_MODEL_OP_END,
};

static int bt_mesh_le_pair_resp_init(struct bt_mesh_model *model)
{
	bt_mesh_le_pair_resp_passkey_invalidate();

	return 0;
}

static void bt_mesh_le_pair_resp_reset(struct bt_mesh_model *model)
{
	bt_mesh_le_pair_resp_passkey_invalidate();
}

const struct bt_mesh_model_cb _bt_mesh_le_pair_resp_cb = {
	.init = bt_mesh_le_pair_resp_init,
	.reset = bt_mesh_le_pair_resp_reset,
};

void bt_mesh_le_pair_resp_passkey_invalidate(void)
{
	(void)bt_passkey_set(BT_PASSKEY_INVALID);
}

void bt_mesh_le_pair_resp_passkey_set(uint32_t passkey)
{
	predefined_passkey = passkey;
}
