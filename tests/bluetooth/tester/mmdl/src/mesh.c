/* mesh.c - Bluetooth Mesh Tester */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/access.h>
#include <zephyr/bluetooth/mesh/cfg.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <app_keys.h>
#include "access.h"

#define LOG_LEVEL CONFIG_BT_MESH_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bttester_mesh);
#include "mesh/testing.h"
#include "model_handler.h"
#include "bttester.h"

#define CONTROLLER_INDEX 0

/* Address of the Node that bound AppKeys to our models (PTS). It is the
 * destination for Client model messages. It is stored in settings because
 * without reprovisioning PTS does not bind AppKeys again after a reboot.
 */
#define TESTER_PTS_ADDR_KEY "bttester/pts_addr"

/* Test state clean-up for the no-reprovisioning mode.
 *
 * Deleted before the "settings_load()", keys relative to "bt/mesh":
 * - s/<id>/{data,pub,sub,subv}, v/<id>/{data,pub,sub,subv}: model state,
 *   publications and subscriptions;
 * - Va/<idx>: label UUIDs, referenced by pub and subv;
 * - Cfg: Configuration Server state (Default TTL, relay, ...);
 * - HBPub: Heartbeat publication.
 * Kept: Net, NetKey, AppKey, IV, Seq, RPL, AppKey bindings, thus can
 * continue without provisioning.
 *
 * Only at the start of a Test Case: Auto-PTS registers the MMDL service
 * (BTP_SERVICE_ID_MMDL) only in the Test Case pre-run.
 * After an IUT reset inside a Test Case (autopts/wid/mmdl.py iut_reset():
 * WID 630, 631, 661, 663) GAP and MESH are registered again, and MESH_INIT,
 * MESH_CONFIG_PROVISIONING and MESH_START are sent again, but not the MMDL
 * service registration. So the states stored before the reset are loaded,
 * as the Test Case expects.
 *
 * Done in MESH_START before settings_load(), so the mesh stack loads the
 * models with their default states.
 */
#define TEST_STATE_KEYS_MAX 64
#define TEST_STATE_KEY_LEN  48

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

struct net_ctx net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

struct test_state_keys {
	size_t cnt;
	bool is_overflow;
	bool is_name_err;
	char names[TEST_STATE_KEYS_MAX][TEST_STATE_KEY_LEN];
};

static struct test_state_keys state_keys;

/* Set by the MMDL service registration, used by the next MESH_START.
 * RAM only: an IUT reset clears it.
 */
static bool has_test_state_clear_req;

uint16_t tester_pts_addr = BT_MESH_ADDR_UNASSIGNED;

static bool tester_mesh_is_key_field(const char *field, const char *name)
{
	const size_t len = strlen(name);

	return !strncmp(field, name, len) &&
	       (field[len] == '\0' || field[len] == '/');
}

static bool tester_mesh_is_test_state_key(const char *key)
{
	const char *field;

	/* Top-level keys: Configuration Server state, Heartbeat, publication and
	 * label UUIDs.
	 */
	if (!strcmp(key, "Cfg") || !strcmp(key, "HBPub") || !strncmp(key, "Va/", 3)) {
		return true;
	}

	/* Model keys: "s/<id>/<field>[/...]" (SIG) or "v/<id>/<field>[/...]"
	 * (vendor).
	 */
	if ((key[0] != 's' && key[0] != 'v') || key[1] != '/') {
		return false;
	}

	/* Skip "<id>/" to get to the field name. */
	field = strchr(key + 2, '/');
	if (!field) {
		return false;
	}
	++field;

	/* Check if this specific entry is one of the Model state, publication and
	 * subscription entries.
	 */
	return tester_mesh_is_key_field(field, "data") ||
	       tester_mesh_is_key_field(field, "pub") ||
	       tester_mesh_is_key_field(field, "sub") ||
	       tester_mesh_is_key_field(field, "subv");
}

static void tester_mesh_set_pts_addr(uint16_t pts_addr)
{
	if (tester_pts_addr == pts_addr) {
		return;
	}

	tester_pts_addr = pts_addr;

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		if (pts_addr == BT_MESH_ADDR_UNASSIGNED) {
			(void)settings_delete(TESTER_PTS_ADDR_KEY);
		} else {
			(void)settings_save_one(TESTER_PTS_ADDR_KEY, &tester_pts_addr,
						sizeof(tester_pts_addr));
		}
	}
}

static void tester_mesh_clear_net_ctx(void)
{
	net.local = BT_MESH_ADDR_UNASSIGNED;
	net.dst = BT_MESH_ADDR_UNASSIGNED;
	net.net_idx = 0;
}

static int tester_mesh_collect_test_state_key(const char *key, size_t len,
					      settings_read_cb read_cb,
					      void *args, void *param)
{
	struct test_state_keys *keys = param;
	int n;

	ARG_UNUSED(len);
	ARG_UNUSED(read_cb);
	ARG_UNUSED(args);

	if (!key || !tester_mesh_is_test_state_key(key)) {
		return 0;
	}

	if (keys->cnt >= TEST_STATE_KEYS_MAX) {
		keys->is_overflow = true;
		return 0;
	}

	n = snprintk(keys->names[keys->cnt], TEST_STATE_KEY_LEN, "bt/mesh/%s",
		     key);
	if (n < 0 || n >= TEST_STATE_KEY_LEN) {
		keys->is_name_err = true;
		return 0;
	}

	++keys->cnt;

	return 0;
}

static int tester_mesh_clear_test_state(void)
{
	int err;

	/* Remove after each walk: deleting inside the load callback is not safe.
	 * Walk on and on until all keys are deleted (the overflow flag is cleared).
	 */
	do {
		state_keys.cnt = 0U;
		state_keys.is_overflow = false;
		state_keys.is_name_err = false;

		err = settings_load_subtree_direct("bt/mesh", tester_mesh_collect_test_state_key,
						   &state_keys);
		if (err) {
			LOG_ERR("Failed to read bt/mesh: err %d", err);
			return err;
		}

		if (state_keys.is_name_err) {
			return -ENAMETOOLONG;
		}

		for (size_t i = 0; i < state_keys.cnt; i++) {
			LOG_DBG("Deleting %s...", state_keys.names[i]);

			err = settings_delete(state_keys.names[i]);
			if (err) {
				LOG_ERR("Failed to delete %s: err %d", state_keys.names[i], err);
				return err;
			}
		}
	} while (state_keys.is_overflow);

	return 0;
}

static int tester_mesh_load_settings(const char *name, size_t len,
				     settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	ssize_t rc;

	if (!settings_name_steq(name, "pts_addr", &next) || next) {
		return -ENOENT;
	}

	if (len != sizeof(tester_pts_addr)) {
		return -EINVAL;
	}

	rc = read_cb(cb_arg, &tester_pts_addr, sizeof(tester_pts_addr));

	return (rc < 0) ? (int)rc : 0;
}

static bool tester_mesh_restore_net_ctx(void)
{
	uint16_t net_idxs[CONFIG_BT_MESH_SUBNET_COUNT];
	uint16_t primary = bt_mesh_primary_addr();
	ssize_t net_cnt;

	if (primary == BT_MESH_ADDR_UNASSIGNED) {
		LOG_ERR("No primary address, cannot proceed.");
		return false;
	}

	/* Model messages use the subnet of their AppKey instead. */
	net_cnt = bt_mesh_subnets_get(net_idxs, ARRAY_SIZE(net_idxs), 0);
	if (net_cnt <= 0) {
		LOG_ERR("No known subnets, cannot proceed.");
		return false;
	}

	/* Select the default subnet for the network context: the lowest NetKey
	 * index, the choice will not depend on the order how subnets were added.
	 * Client models use the subnet of their AppKey.
	 */
	net.net_idx = net_idxs[0];
	for (ssize_t i = 1; i < net_cnt; i++) {
		if (net_idxs[i] < net.net_idx) {
			net.net_idx = net_idxs[i];
		}
	}

	net.local = primary;

	return true;
}

SETTINGS_STATIC_HANDLER_DEFINE(bttester, "bttester", NULL, tester_mesh_load_settings,
			       NULL, NULL);

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

static int output_numeric(bt_mesh_output_action_t action, uint8_t *numeric, size_t size)
{
	struct mesh_out_number_action_ev ev;
	uint32_t number;

	if (size > sizeof(number)) {
		LOG_ERR("Unsupported size %zu", size);
		return -EINVAL;
	}

	number = sys_get_le32(numeric);

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
	.output_numeric = output_numeric,
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
	bool is_ctx_restored = false;
	int err;

	LOG_DBG("");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		/* Each Test Case starts with default model states. Not done after
		 * an IUT reset inside a Test Case (no MMDL service registration).
		 * If the node is already running (no IUT reset before the Test
		 * Case), MESH_START fails: the states cannot be cleared.
		 */
		if (has_test_state_clear_req) {
			if (bt_mesh_is_provisioned()) {
				LOG_ERR("Test state clear requested on a running node: "
					"IUT was not reset before the Test Case.");
				status = BTP_STATUS_FAILED;
			} else {
				err = tester_mesh_clear_test_state();
				if (err) {
					LOG_ERR("Failed to clear test state: err %d", err);
					status = BTP_STATUS_FAILED;
				}
			}
			has_test_state_clear_req = false;
		}

		settings_load();
	}

	/* Node already in a network (flash or current session): do not re-open PB-ADV. */
	if (!bt_mesh_is_provisioned()) {
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
	} else {
		if (!tester_mesh_restore_net_ctx()) {
			status = BTP_STATUS_FAILED;
		} else {
			is_ctx_restored = true;
		}
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_START, CONTROLLER_INDEX, status);

	/* Context restored from RRAM: let auto-pts get the PROVISIONED event,
	 * WID13 resets the Node in a case of absent keys.
	 */
	if (is_ctx_restored) {
		tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROVISIONED, CONTROLLER_INDEX,
			    NULL, 0);
	}
}

static void reset(uint8_t *data, uint16_t len)
{
	LOG_DBG("");

	tester_mesh_clear_net_ctx();
	tester_mesh_set_pts_addr(BT_MESH_ADDR_UNASSIGNED);
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
	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, (void *)model);

	/* Do not touch model publication here: a server model with publication
	 * set sends an extra status on every state change.
	 */
	tester_mesh_set_pts_addr(addr);
}

static void model_unbound_cb(uint16_t addr, const struct bt_mesh_model *model,
			     uint16_t key_idx)
{
	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, (void *)model);
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

bool tester_mesh_get_net_idx_from_appkey_idx(uint16_t akey_idx, uint16_t *nkey_idx)
{
	uint16_t net_idxs[CONFIG_BT_MESH_SUBNET_COUNT];
	uint16_t appkeys_idxs[CONFIG_BT_MESH_APP_KEY_COUNT];
	ssize_t appkeys_cnt;

	/* Get all known subnets, do not proceed if there are no known ones. */
	ssize_t net_cnt = bt_mesh_subnets_get(net_idxs, ARRAY_SIZE(net_idxs), 0);

	if (net_cnt <= 0) {
		LOG_ERR("No known subnets, cannot proceed.");
		return false;
	}

	/* Walk through all known subnets. */
	for (ssize_t i = 0; i < net_cnt; i++) {
		/* Get all appkeys bound to the current subnet. */
		appkeys_cnt = bt_mesh_app_keys_get(net_idxs[i], appkeys_idxs,
						   ARRAY_SIZE(appkeys_idxs), 0);
		if (appkeys_cnt <= 0) {
			continue;
		}
		/* Walk through all appkeys bound to this subnet, try to solve. */
		for (ssize_t j = 0; j < appkeys_cnt; j++) {
			if (appkeys_idxs[j] == akey_idx) {
				*nkey_idx = net_idxs[i];
				return true;
			}
		}
	}

	return false;
}

void tester_mesh_set_state_reset_request(void)
{
	has_test_state_clear_req = true;
}

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
