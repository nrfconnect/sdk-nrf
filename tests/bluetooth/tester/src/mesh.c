/* mesh.c - Bluetooth Mesh Tester */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <assert.h>
#include <errno.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/cfg.h>
#include <zephyr/sys/byteorder.h>
#include <app_keys.h>

#define LOG_LEVEL CONFIG_BT_MESH_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bttester_mesh);
#include "mesh/testing.h"
#include "model_handler.h"
#include "bttester.h"

#define CONTROLLER_INDEX 0

/* Health server data */
extern uint8_t cur_faults[CUR_FAULTS_MAX];
extern uint8_t reg_faults[CUR_FAULTS_MAX * 2];
extern struct bt_mesh_model vnd_models[1];

/* Provision node data */
static uint8_t net_key[16];
static uint16_t net_key_idx;
static uint8_t flags;
static uint32_t iv_index;
static uint16_t addr;
static uint8_t dev_key[16];
static uint8_t input_size;

/* Configured provisioning data */
static uint8_t dev_uuid[16];
static uint8_t static_auth[16];

/* Model send data */
struct model_data model_bound[MODEL_BOUNDS_MAX];

struct net_ctx net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

static void supported_commands(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);

	/* 1st octet */
	net_buf_simple_add_u8(buf, 0);
	tester_set_bit(buf->data, MESH_READ_SUPPORTED_COMMANDS);
	tester_set_bit(buf->data, MESH_CONFIG_PROVISIONING);
	tester_set_bit(buf->data, MESH_INIT);
	tester_set_bit(buf->data, MESH_RESET);
	tester_set_bit(buf->data, MESH_START);

	tester_send(BTP_SERVICE_ID_MESH, MESH_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, buf->data, buf->len);
}

static void link_open(bt_mesh_prov_bearer_t bearer)
{
	struct mesh_prov_link_open_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROV_LINK_OPEN,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	struct mesh_prov_link_closed_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROV_LINK_CLOSED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	struct mesh_out_number_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%08x", action, number);

	ev.action = sys_cpu_to_le16(action);
	ev.number = sys_cpu_to_le32(number);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_NUMBER_ACTION,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));

	return 0;
}

static int output_string(const char *str)
{
	struct mesh_out_string_action_ev *ev;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	LOG_DBG("str %s", str);

	net_buf_simple_init(buf, 0);

	ev = net_buf_simple_add(buf, sizeof(*ev));
	ev->string_len = strlen(str);

	net_buf_simple_add_mem(buf, str, ev->string_len);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_STRING_ACTION,
		    CONTROLLER_INDEX, buf->data, buf->len);

	return 0;
}

static int input(bt_mesh_input_action_t action, uint8_t size)
{
	struct mesh_in_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%02x", action, size);

	input_size = size;

	ev.action = sys_cpu_to_le16(action);
	ev.size = size;

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_IN_ACTION, CONTROLLER_INDEX,
		    (uint8_t *) &ev, sizeof(ev));

	return 0;
}

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
	LOG_DBG("net_idx 0x%04x addr 0x%04x", net_idx, addr);

	net.net_idx = net_idx,
	net.local = addr;
	net.dst = addr;

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROVISIONED, CONTROLLER_INDEX,
		    NULL, 0);
}

static void prov_reset(void)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
	if (err && err != -EALREADY) {
		LOG_ERR("prov enable error %d", err);
	}
}

static const struct bt_mesh_comp *comp;

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.static_val = static_auth,
	.static_val_len = sizeof(static_auth),
	.output_number = output_number,
	.output_string = output_string,
	.input = input,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.reset = prov_reset,
};

static void config_prov(uint8_t *data, uint16_t len)
{
	const struct mesh_config_provisioning_cmd *cmd = (void *) data;

	LOG_DBG("");

	memcpy(dev_uuid, cmd->uuid, sizeof(dev_uuid));
	memcpy(static_auth, cmd->static_auth, sizeof(static_auth));

	prov.output_size = cmd->out_size;
	prov.output_actions = sys_le16_to_cpu(cmd->out_actions);
	prov.input_size = cmd->in_size;
	prov.input_actions = sys_le16_to_cpu(cmd->in_actions);

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CONFIG_PROVISIONING,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

static void init(uint8_t *data, uint16_t len)
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	LOG_DBG("");

	comp = model_handler_init();
	err = bt_mesh_init(&prov, comp);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INIT, CONTROLLER_INDEX,
		   status);
}

static void start(uint8_t *data, uint16_t len)
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		printk("Loading stored settings\n");
		settings_load();
	}

	if (addr) {
		err = bt_mesh_provision(net_key, net_key_idx, flags, iv_index,
					addr, dev_key);
		if (err && err != -EALREADY) {
			status = BTP_STATUS_FAILED;
		}
	} else {
		err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
		if (err && err != -EALREADY) {
			status = BTP_STATUS_FAILED;
		}
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_START, CONTROLLER_INDEX,
		   status);
}

static void reset(uint8_t *data, uint16_t len)
{
	LOG_DBG("");

	bt_mesh_reset();

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_RESET, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

void tester_handle_mesh(uint8_t opcode, uint8_t index, uint8_t *data, uint16_t len)
{
	switch (opcode) {
	case MESH_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		break;
	case MESH_CONFIG_PROVISIONING:
		config_prov(data, len);
		break;
	case MESH_INIT:
		init(data, len);
		break;
	case MESH_START:
		start(data, len);
		break;
	case MESH_RESET:
		reset(data, len);
		break;
	default:
		tester_rsp(BTP_SERVICE_ID_MESH, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

void net_recv_ev(uint8_t ttl, uint8_t ctl, uint16_t src, uint16_t dst, const void *payload,
		 size_t payload_len)
{
	NET_BUF_SIMPLE_DEFINE(buf, UINT8_MAX);
	struct mesh_net_recv_ev *ev;

	LOG_DBG("ttl 0x%02x ctl 0x%02x src 0x%04x dst 0x%04x payload_len %zu",
		ttl, ctl, src, dst, payload_len);

	if (payload_len > net_buf_simple_tailroom(&buf)) {
		LOG_ERR("Payload size exceeds buffer size");
		return;
	}

	ev = net_buf_simple_add(&buf, sizeof(*ev));
	ev->ttl = ttl;
	ev->ctl = ctl;
	ev->src = sys_cpu_to_le16(src);
	ev->dst = sys_cpu_to_le16(dst);
	ev->payload_len = payload_len;
	net_buf_simple_add_mem(&buf, payload, payload_len);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_NET_RECV, CONTROLLER_INDEX,
		    buf.data, buf.len);
}

static void model_bound_cb(uint16_t addr, const struct bt_mesh_model *model,
			   uint16_t key_idx)
{
	int i;

	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, (void *)model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (!model_bound[i].model) {
			model_bound[i].model = model;
			model_bound[i].addr = addr;
			model_bound[i].appkey_idx = key_idx;

			return;
		}
	}

	LOG_ERR("model_bound is full");
}

static void model_unbound_cb(uint16_t addr, const struct bt_mesh_model *model,
			     uint16_t key_idx)
{
	int i;

	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, (void *)model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (model_bound[i].model == model) {
			model_bound[i].model = NULL;
			model_bound[i].addr = 0x0000;
			model_bound[i].appkey_idx = BT_MESH_KEY_UNUSED;

			return;
		}
	}

	LOG_INF("model not found");
}

static void invalid_bearer_cb(uint8_t opcode)
{
	struct mesh_invalid_bearer_ev ev = {
		.opcode = opcode,
	};

	LOG_DBG("opcode 0x%02x", opcode);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_INVALID_BEARER,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void incomp_timer_exp_cb(void)
{
	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_INCOMP_TIMER_EXP,
		    CONTROLLER_INDEX, NULL, 0);
}

static struct bt_mesh_test_cb mesh_test_cb = {
	.net_recv = net_recv_ev,
	.model_bound = model_bound_cb,
	.model_unbound = model_unbound_cb,
	.prov_invalid_bearer = invalid_bearer_cb,
	.trans_incomp_timer_exp = incomp_timer_exp_cb,
};

uint8_t tester_init_mesh(void)
{
	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_mesh_test_cb_register(&mesh_test_cb);
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mesh(void)
{
	return BTP_STATUS_SUCCESS;
}
