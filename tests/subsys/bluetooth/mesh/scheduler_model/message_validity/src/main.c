/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>
#include <bluetooth/mesh/gen_onoff_srv.h>
#include <bluetooth/mesh/time_srv.h>
#include <bluetooth/mesh/scheduler_srv.h>
#include <bluetooth/mesh/scheduler_cli.h>
#include <scheduler_internal.h>
#include <model_utils.h>
#include <time_util.h>
#include <sched_test.h>

#define TIMEOUT 5

static uint8_t test_idx;

static const struct bt_mesh_schedule_entry valid_entry = {
	.year = 0x63,
	.month = ANY_MONTH,
	.day = 9,
	.hour = 9,
	.minute = 9,
	.second = 9,
	.day_of_week = ANY_DAY_OF_WEEK,
	.action = BT_MESH_SCHEDULER_TURN_ON,
	.transition_time = 100,
	.scene_number = 1,
};

struct bt_mesh_msg_ctx test_ctx = {
	.addr = 1,
};

static struct bt_mesh_scene_srv scene_srv;

static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);

static void onoff_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      const struct bt_mesh_onoff_set *set, struct bt_mesh_onoff_status *rsp)
{
}

static void onoff_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_onoff_status *rsp)
{
}

struct bt_mesh_onoff_srv_handlers onoff_srv_handlers = {
	.set = onoff_set,
	.get = onoff_get,
};

struct bt_mesh_onoff_srv onoff_srv = BT_MESH_ONOFF_SRV_INIT(&onoff_srv_handlers);

static void srv_action_set_cb(struct bt_mesh_scheduler_srv *srv, struct bt_mesh_msg_ctx *ctx,
			      uint8_t idx, struct bt_mesh_schedule_entry *entry)
{
	ztest_check_expected_value(srv);
	ztest_check_expected_value(ctx);
	ztest_check_expected_value(idx);
	ztest_check_expected_data(entry, sizeof(struct bt_mesh_schedule_entry));
}

static struct bt_mesh_scheduler_srv sched_srv =
	BT_MESH_SCHEDULER_SRV_INIT(&srv_action_set_cb, &time_srv);

static void cli_action_status_handler(struct bt_mesh_scheduler_cli *cli,
				      struct bt_mesh_msg_ctx *ctx, uint8_t idx,
				      const struct bt_mesh_schedule_entry *action)
{
	ztest_check_expected_value(cli);
	ztest_check_expected_value(ctx);
	ztest_check_expected_value(idx);
	ztest_check_expected_data(action, sizeof(struct bt_mesh_schedule_entry));
}

static void cli_status_handler(struct bt_mesh_scheduler_cli *cli, struct bt_mesh_msg_ctx *ctx,
			       uint16_t schedules)
{
	ztest_check_expected_value(cli);
	ztest_check_expected_value(ctx);
	ztest_check_expected_value(schedules);
}

static struct bt_mesh_scheduler_cli sched_cli = {
	.action_status_handler = &cli_action_status_handler,
	.status_handler = &cli_status_handler,
};

/** Mocking mesh composition data
 *
 * The mocked composition data consists of 1 element with
 * 1 Scheduler server, 1 Scheduler client, 1 Generic onoff server,
 * and 1 Scene server.
 *
 * |      1     |
 * |:----------:|
 * | Sched. Srv |
 * | Sched. Cli |
 * | Onoff Srv  |
 * | Scene Srv  |
 *
 */
static struct bt_mesh_model mock_models_elem[4] = {
	{
		.user_data = &sched_srv,
		.id = BT_MESH_MODEL_ID_SCHEDULER_SRV,
	},
	{
		.user_data = &sched_cli,
		.id = BT_MESH_MODEL_ID_SCHEDULER_CLI,
	},
	{
		.user_data = &onoff_srv,
		.id = BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
	},
	{
		.user_data = &scene_srv,
		.id = BT_MESH_MODEL_ID_SCENE_SRV,
	},
};

static struct bt_mesh_elem mock_elem = {
	.addr = 1,
	.model_count = ARRAY_SIZE(mock_models_elem),
	.models = mock_models_elem,
};

struct mock_comp_data {
	size_t elem_count;
	struct bt_mesh_elem *elem;
} mock_comp = { .elem_count = 1, .elem = &mock_elem };

static struct bt_mesh_model *mock_mod_srv = &mock_models_elem[0];
static struct bt_mesh_model *mock_mod_cli = &mock_models_elem[1];

/* Redefined mocks */

struct bt_mesh_elem *bt_mesh_model_elem(struct bt_mesh_model *mod)
{
	return &mock_comp.elem[0];
}

struct bt_mesh_elem *bt_mesh_elem_find(uint16_t addr)
{
	return (addr == mock_comp.elem[0].addr ? &mock_comp.elem[0] : NULL);
}

struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem, uint16_t id)
{
	uint8_t i;

	for (i = 0U; i < elem->model_count; i++) {
		if (elem->models[i].id == id) {
			return &elem->models[i];
		}
	}
	return NULL;
}

int bt_mesh_scene_srv_set(struct bt_mesh_scene_srv *srv, uint16_t scene,
			  struct bt_mesh_model_transition *transition)
{
	return 0;
}

struct tm *bt_mesh_time_srv_localtime(struct bt_mesh_time_srv *srv, int64_t uptime)
{
	static struct tm timeptr;
	struct bt_mesh_time_tai tai;
	int64_t tmp = tai_to_ms(&tai);

	tmp += uptime;
	tai = tai_at(tmp);
	tai_to_ts(&tai, &timeptr);

	return &timeptr;
}

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
}

int64_t bt_mesh_time_srv_mktime(struct bt_mesh_time_srv *srv, struct tm *timeptr)
{
	static const struct tm start_tm = {
		/* 1st of Jan 2010 */
		.tm_year = 110,
		.tm_mon = 0, .tm_mday = 1,
		.tm_wday = 5,
		.tm_hour = 0,
		.tm_min = 0,
		.tm_sec = 0,
	};

	return (timeutil_timegm64(timeptr) - timeutil_timegm64(&start_tm)) * 1000;
}

int bt_mesh_scene_srv_pub(struct bt_mesh_scene_srv *srv, struct bt_mesh_msg_ctx *ctx)
{
	return 0;
}

int bt_mesh_onoff_srv_pub(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_status *status)
{
	return 0;
}

void bt_mesh_time_encode_time_ress(struct net_buf_simple *buf,
				   const struct bt_mesh_time_status *status)
{
}

int bt_mesh_msg_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	ztest_check_expected_value(model);
	ztest_check_expected_value(ctx);
	/* action is packed, check buffer length only */
	ztest_check_expected_value(buf->len);

	return 0;
}

int bt_mesh_msg_ackd_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf, const struct bt_mesh_msg_rsp_ctx *rsp)
{
	ztest_check_expected_value(model);
	ztest_check_expected_value(ctx);
	ztest_check_expected_value(rsp);
	/* action is packed, check buffer length only */
	ztest_check_expected_value(buf->len);
	return 0;
}

int _bt_mesh_time_srv_update_handler(struct bt_mesh_model *model)
{
	return 0;
}

bool bt_mesh_msg_ack_ctx_match(const struct bt_mesh_msg_ack_ctx *ack, uint32_t op, uint16_t addr,
			       void **user_data)
{
	return false;
}


static void assert_cli_action_set_send(const struct bt_mesh_schedule_entry *test_entry,
				       uint8_t test_idx, int ret_val)
{
	zassert_equal(bt_mesh_scheduler_cli_action_set(&sched_cli, NULL, test_idx, test_entry,
						       NULL),
		      ret_val);
	zassert_equal(bt_mesh_scheduler_cli_action_set_unack(&sched_cli, NULL, test_idx,
							     test_entry),
		      ret_val);
}

/* Prepare buffer and call server SET opcode handler */
static int action_set(const struct bt_mesh_schedule_entry *test_entry, bool ack)
{
	BT_MESH_MODEL_BUF_DEFINE(
		buf, ack ? BT_MESH_SCHEDULER_OP_ACTION_SET : BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
		BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);

	scheduler_action_pack(&buf, test_idx, test_entry);

	return _bt_mesh_scheduler_setup_srv_op[ack ? 0 : 1].func(sched_srv.model, NULL, &buf);
}

/* Prepare buffer and call server GET opcode handler */
static int current_register_get(void)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_GET, BT_MESH_SCHEDULER_MSG_LEN_GET);

	return _bt_mesh_scheduler_srv_op[0].func(sched_srv.model, NULL, &buf);
}

/* Prepare buffer and call server ACTION GET opcode handler */
static int action_get(void)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_GET,
				 BT_MESH_SCHEDULER_MSG_LEN_ACTION_GET);

	net_buf_simple_add_u8(&buf, test_idx);

	return _bt_mesh_scheduler_srv_op[1].func(sched_srv.model, NULL, &buf);
}

static void *setup(void)
{
	zassert_not_null(_bt_mesh_scheduler_srv_cb.init, "Server init cb is null");
	zassert_not_null(_bt_mesh_scheduler_cli_cb.init, "Client init cb is null");

	_bt_mesh_scheduler_srv_cb.init(mock_mod_srv);
	_bt_mesh_scheduler_cli_cb.init(mock_mod_cli);

	return NULL;
}

static void run_before(void *f)
{
	test_idx = 0;
}

static void teardown(void *f)
{
	zassert_not_null(_bt_mesh_scheduler_srv_cb.reset, "Server reset cb is null");
	zassert_not_null(_bt_mesh_scheduler_cli_cb.reset, "Client reset cb is null");

	_bt_mesh_scheduler_srv_cb.reset(mock_mod_srv);
	_bt_mesh_scheduler_cli_cb.reset(mock_mod_cli);
}

ZTEST_SUITE(scheduler_message_validity_test, NULL, setup, run_before, NULL, teardown);

ZTEST(scheduler_message_validity_test, test_cli_set_prohibited)
{
	/** (1) Attempt to send a SET/SET_UNACK-message with a prohibited test action entry,
	 * verify that the client returns with error.
	 * (2) Verify that no message is being sent.
	 */

	struct bt_mesh_schedule_entry test_entry[3];

	for (int i = 0; i < 3; i++) {
		test_entry[i] = valid_entry;
	}

	/* prohibited struct member values */
	test_entry[0].year = 101;
	test_entry[1].hour = 26;
	test_entry[2].action = 5;

	for (int i = 0; i < ARRAY_SIZE(test_entry); i++) {
		assert_cli_action_set_send(&test_entry[i], test_idx, -EINVAL);
	}

	/* test with NULL-pointer */
	assert_cli_action_set_send(NULL, test_idx, -EINVAL);

	/** This test will fail if either bt_mesh_msg_send() or bt_mesh_msg_ackd_send() is called
	 * as there are no expected values for these functions.
	 */
}

ZTEST(scheduler_message_validity_test, test_cli_set_valid)
{
	/** Verify that the client accepts a valid entry and proceeds to send the SET-message(s),
	 * both acknowledged and unacknowledged, with success.
	 */

	ztest_expect_value(bt_mesh_msg_send, model, sched_cli.model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);

	ztest_expect_value(bt_mesh_msg_ackd_send, model, sched_cli.model);
	ztest_expect_value(bt_mesh_msg_ackd_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_ackd_send, rsp, NULL);
	ztest_expect_value(bt_mesh_msg_ackd_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);

	assert_cli_action_set_send(&valid_entry, test_idx, 0);
}

ZTEST(scheduler_message_validity_test, test_cli_get_action_prohibited)
{
	/** (1) Send an ACTION GET message with a prohibited schedule action index count.
	 * (2) Verify that the client denies the prohibited index value
	 * and that no message is sent.
	 */

	test_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	zassert_equal(bt_mesh_scheduler_cli_action_get(&sched_cli, NULL, test_idx, NULL), -EINVAL);

	/** This test will fail if bt_mesh_msg_ackd_send() is called as there are no expected values
	 * for this function.
	 */
}

ZTEST(scheduler_message_validity_test, test_cli_get_action_valid)
{
	/** (1) Send an ACTION GET message with a valid schedule action index count.
	 * (2) Verify that the client accepts the valid index value and proceeds to
	 * successfully send the GET-message.
	 * (3) Verify that the client is able to successfully handle a ACTION STATUS response,
	 * and that the appropriate callback is called with expected values.
	 */

	test_idx = 1;
	ztest_expect_value(bt_mesh_msg_ackd_send, model, sched_cli.model);
	ztest_expect_value(bt_mesh_msg_ackd_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_ackd_send, rsp, NULL);
	ztest_expect_value(bt_mesh_msg_ackd_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_ACTION_GET);

	zassert_ok(bt_mesh_scheduler_cli_action_get(&sched_cli, NULL, test_idx, NULL));

	/* Create a packed action with a valid entry */
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
				 BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK);
	scheduler_action_pack(&buf, test_idx, &valid_entry);

	ztest_expect_value(cli_action_status_handler, ctx, &test_ctx);
	ztest_expect_value(cli_action_status_handler, idx, test_idx);
	ztest_expect_value(cli_action_status_handler, cli, &sched_cli);
	ztest_expect_data(cli_action_status_handler, action, &valid_entry);

	zassert_ok(_bt_mesh_scheduler_cli_op[1].func(sched_cli.model, &test_ctx, &buf));
}

ZTEST(scheduler_message_validity_test, test_cli_get_valid)
{
	/** (1) Send a GET message and ensure that the client succeeds in sending the message.
	 * (2) Verify that the client is able to successfully handle a STATUS response,
	 * and that the appropriate callback is called with expected values.
	 */

	ztest_expect_value(bt_mesh_msg_ackd_send, model, sched_cli.model);
	ztest_expect_value(bt_mesh_msg_ackd_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_ackd_send, rsp, NULL);
	ztest_expect_value(bt_mesh_msg_ackd_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_GET);

	zassert_ok(bt_mesh_scheduler_cli_get(&sched_cli, NULL, NULL));

	NET_BUF_SIMPLE_DEFINE(dummy_rsp, 2);
	net_buf_simple_add_le16(&dummy_rsp, 12345);

	ztest_expect_value(cli_status_handler, cli, &sched_cli);
	ztest_expect_value(cli_status_handler, ctx, &test_ctx);
	ztest_expect_value(cli_status_handler, schedules, 12345);

	zassert_ok(_bt_mesh_scheduler_cli_op[0].func(sched_cli.model, &test_ctx, &dummy_rsp));
}

ZTEST(scheduler_message_validity_test, test_srv_set_prohibited)
{
	/** (1) If server receives a SET/SET_UNACK-message with a prohibited test action entry,
	 * verify that the server's message handler(s) return with error.
	 * (2) Verify that no callback is called.
	 * (3) Verify that no status messages are sent.
	 */

	struct bt_mesh_schedule_entry test_entry[3];

	for (int i = 0; i < 3; i++) {
		test_entry[i] = valid_entry;
	}

	/* prohibited struct member values */
	test_entry[0].year = 101;
	test_entry[1].hour = 26;
	test_entry[2].action = 5;

	for (int i = 0; i < 3; i++) {
		zassert_equal(action_set(&test_entry[i], false), -EINVAL);
		zassert_equal(action_set(&test_entry[i], true), -EINVAL);
	}

	/** This test will fail if either bt_mesh_msg_send(), bt_mesh_msg_ackd_send()
	 * or action_set_cb() is called as there are no expected values for these functions.
	 */
}

ZTEST(scheduler_message_validity_test, test_srv_set_rsp)
{
	/** (1) Upon receiving a valid ACTION_SET-message, verify that the server message handler(s)
	 * return with success.
	 * (2) Check that the appropriate callback is called with expected values.
	 * (3) Verify that the server sends status message by publishing,
	 * and by ctx if acknowledged.
	 * (4) Repeat step 1-3 for ACTION_SET_UNACK
	 */

	/* Acked */
	ztest_expect_value(srv_action_set_cb, srv, &sched_srv);
	ztest_expect_value(srv_action_set_cb, ctx, NULL);
	ztest_expect_value(srv_action_set_cb, idx, test_idx);
	ztest_expect_data(srv_action_set_cb, entry, &valid_entry);

	ztest_expect_value(bt_mesh_msg_send, model, sched_srv.model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS);

	/* expect 2nd call to bt_mesh_msg_send */
	ztest_expect_value(bt_mesh_msg_send, model, sched_srv.model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS);

	zassert_ok(action_set(&valid_entry, true));

	/* Unacked */
	ztest_expect_value(srv_action_set_cb, srv, &sched_srv);
	ztest_expect_value(srv_action_set_cb, ctx, NULL);
	ztest_expect_value(srv_action_set_cb, idx, test_idx);
	ztest_expect_data(srv_action_set_cb, entry, &valid_entry);

	ztest_expect_value(bt_mesh_msg_send, model, sched_srv.model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS);

	zassert_ok(action_set(&valid_entry, false));
}

ZTEST(scheduler_message_validity_test, test_srv_get_action_prohibited)
{
	/** (1) If server receives an ACTION GET message with a prohibited action index count,
	 * verify that the server message handler returns with error.
	 * (2) Check that no status message is sent.
	 */

	test_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	zassert_equal(action_get(), -ENOENT);

	/** This test will fail if bt_mesh_msg_send() is called as there are no expected values
	 * for this function.
	 */
}

ZTEST(scheduler_message_validity_test, test_srv_get_action_rsp)
{
	/** (1) Upon receiving a valid GET ACTION-message verify that the server accepts
	 * the valid index value.
	 * (2) Check that a status message is sent in response.
	 */

	test_idx = 1;
	ztest_expect_value(bt_mesh_msg_send, model, sched_srv.model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	/* reduced since the action entry is not previously defined */
	ztest_expect_value(bt_mesh_msg_send, buf->len,
			   BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS_REDUCED);

	zassert_ok(action_get());
}

ZTEST(scheduler_message_validity_test, test_srv_get_rsp)
{
	/** Upon receiving a GET message, verify that a status message is sent in response.
	 */

	ztest_expect_value(bt_mesh_msg_send, model, sched_srv.model);
	ztest_expect_value(bt_mesh_msg_send, ctx, NULL);
	ztest_expect_value(bt_mesh_msg_send, buf->len, BT_MESH_SCHEDULER_MSG_LEN_STATUS);

	zassert_ok(current_register_get());
}
