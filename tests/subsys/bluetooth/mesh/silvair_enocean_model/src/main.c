/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <bluetooth/enocean.h>
#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/vnd/silvair_enocean_srv.h>

/** Definitions from specification: */
#define PHASE_B_DURATION_MS 150UL
#define DELTA_TIMEOUT_MS 6000UL
#define WAIT_TIME_MS 400UL
#define LEVEL_DELTA 2520
#define LEVEL_TRANSITION_TIME_MS 200UL

/** Should send until more than six seconds have passed since button down.
 *  Phase C entered after WAIT_TIME_MS + PHASE_B_DURATION_MS, meaning:
 */
#define MAX_PHASE_C_DURATION (DELTA_TIMEOUT_MS - (WAIT_TIME_MS + PHASE_B_DURATION_MS))
/** +4 ticks for phase B */
#define MAX_TICKS ((MAX_PHASE_C_DURATION / 100) + 4)

#define DELAY_TIME_STEP_MS (5)

/** Mocks ******************************************/

static struct bt_mesh_silvair_enocean_srv srv;

static struct bt_enocean_device mock_enocean_device = {
	.addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {
			.val = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}
		}
	}
};

static const struct bt_mesh_model mock_model = {
	.rt = &(struct bt_mesh_model_rt_ctx){.user_data = &srv}
};

static const struct bt_mesh_model mock_lvl_model = {
	.pub = &srv.buttons[0].lvl.pub,
	.rt = &(struct bt_mesh_model_rt_ctx){.user_data = &srv.buttons[0].lvl},
};

static const struct bt_mesh_model mock_onoff_model = {
	.pub = &srv.buttons[0].onoff.pub,
	.rt = &(struct bt_mesh_model_rt_ctx){.user_data = &srv.buttons[0].onoff}
};

static const struct bt_enocean_callbacks *enocean_cbs;

static struct {
	k_work_handler_t handler;
	struct k_work_delayable *dwork;
} mock_timers[BT_MESH_SILVAIR_ENOCEAN_PROXY_BUTTONS];

static bool commisioning_enabled;

void bt_enocean_init(const struct bt_enocean_callbacks *cbs)
{
	enocean_cbs = cbs;
}

void bt_enocean_decommission(struct bt_enocean_device *device)
{
	ztest_check_expected_value(device);
}

int bt_enocean_commission(const bt_addr_le_t *addr, const uint8_t key[16],
			  uint32_t seq)
{
	ztest_check_expected_data(key, 16);
	ztest_check_expected_data(addr, sizeof(bt_addr_le_t));
	return ztest_get_return_value();
}

void bt_enocean_commissioning_disable(void)
{
	commisioning_enabled = false;
}

void bt_enocean_commissioning_enable(void)
{
	commisioning_enabled = true;
}

uint32_t bt_enocean_foreach(bt_enocean_foreach_cb_t cb, void *user_data)
{
	if (cb) {
		cb(&mock_enocean_device, user_data);
		return 1;
	}
	return 0;
}

uint8_t model_delay_encode(uint32_t delay)
{
	return delay / DELAY_TIME_STEP_MS;
}

int32_t model_delay_decode(uint8_t encoded_delay)
{
	return encoded_delay * DELAY_TIME_STEP_MS;
}

const struct bt_mesh_elem *bt_mesh_model_elem(const struct bt_mesh_model *mod)
{
	return NULL;
}

const struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
					       uint16_t id)
{
	return NULL;
}

int bt_mesh_onoff_cli_set_unack(struct bt_mesh_onoff_cli *cli,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_onoff_set *set)
{
	ztest_check_expected_value(cli);
	ztest_check_expected_value(set->on_off);
	ztest_check_expected_value(set->reuse_transaction);
	zassert_not_null(set->transition, "transition is null");
	ztest_check_expected_value(set->transition->time);
	ztest_check_expected_value(set->transition->delay);
	ztest_check_expected_value(ctx->send_ttl);
	ztest_check_expected_value(ctx->app_idx);
	ztest_check_expected_value(ctx->addr);
	return 0;
}

int bt_mesh_lvl_cli_delta_set_unack(
		struct bt_mesh_lvl_cli *cli,
		struct bt_mesh_msg_ctx *ctx,
		const struct bt_mesh_lvl_delta_set *delta_set)
{
	ztest_check_expected_value(cli);
	ztest_check_expected_value(delta_set->delta);
	ztest_check_expected_value(delta_set->new_transaction);
	zassert_not_null(delta_set->transition, "transition is null");
	ztest_check_expected_value(delta_set->transition->time);
	ztest_check_expected_value(delta_set->transition->delay);
	ztest_check_expected_value(ctx->send_ttl);
	ztest_check_expected_value(ctx->app_idx);
	ztest_check_expected_value(ctx->addr);
	return 0;
}

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
	ztest_check_expected_value(opcode);
	net_buf_simple_init(msg, 0);
	/* Opcodes sent by enocean model are always 3 octets */
	net_buf_simple_add_u8(msg, ((opcode >> 16) & 0xff));
	net_buf_simple_add_le16(msg, opcode & 0xffff);
}

int bt_mesh_model_data_store(const struct bt_mesh_model *model, bool vnd,
			     const char *name, const void *data,
			     size_t data_len)
{
	zassert_equal(model, &mock_model, "Incorrect model");
	zassert_true(vnd, "vnd is not true");
	zassert_is_null(name, "unexpected name supplied");
	ztest_check_expected_value(data_len);
	ztest_check_expected_data(data, data_len);
	return 0;
}

int bt_mesh_msg_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf)
{
	ztest_check_expected_value(model);
	ztest_check_expected_value(ctx);
	ztest_check_expected_value(buf->len);
	ztest_check_expected_data(buf->data, buf->len);
	return 0;
}


void k_work_init_delayable(struct k_work_delayable *dwork,
				  k_work_handler_t handler)
{
	for (int i = 0; i < BT_MESH_SILVAIR_ENOCEAN_PROXY_BUTTONS; i++) {
		if (dwork == &srv.buttons[i].timer) {
			mock_timers[i].handler = handler;
			mock_timers[i].dwork = dwork;
			return;
		}
	}
	ztest_test_fail();
}

int k_work_cancel_delayable(struct k_work_delayable *dwork)
{
	ztest_check_expected_value(dwork);
	return 0;
}

static bool delta_timedout;

int k_work_delayable_busy_get(const struct k_work_delayable *dwork)
{
	return delta_timedout ? 0 : 1;
}

/*
 * This is mocked, as k_work_reschedule is inline and can't be, but calls this
 * underneath
 */
int k_work_reschedule_for_queue(struct k_work_q *queue,
				struct k_work_delayable *dwork,
				k_timeout_t delay)
{
	zassert_equal(queue, &k_sys_work_q, "Not rescheduled on k_sys_work_q");
	ztest_check_expected_value(dwork);
	ztest_check_expected_data(&delay, sizeof(delay));
	return 0;
}

void *settings_cb_arg = (void *)0xbeef;

static bt_addr_le_t stored_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { .val = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } }
};

static ssize_t mock_read_cb(void *cb_arg, void *data, size_t len)
{
	zassert_equal(cb_arg, settings_cb_arg, "Arg does not match");
	ztest_check_expected_value(len);
	ztest_check_expected_value(data);
	memcpy(data, &stored_addr, sizeof(stored_addr));
	return ztest_get_return_value();
}

/** End of mocks ***********************************/

enum bt_mesh_silvair_enocean_status {
	BT_MESH_SILVAIR_ENOCEAN_STATUS_SET = 0x00,
	BT_MESH_SILVAIR_ENOCEAN_STATUS_NOT_SET = 0x01,
	BT_MESH_SILVAIR_ENOCEAN_STATUS_UNSPECIFIED_ERROR = 0x02,
};

static void expect_status(struct bt_mesh_msg_ctx *ctx,
			  enum bt_mesh_silvair_enocean_status status,
			  const uint8_t *addr)
{
	static uint8_t expected_msg[11] = {0xF4, 0x36, 0x01, 0x03};
	size_t len = 4;

	expected_msg[len++] = status;
	if (addr) {
		memcpy(&expected_msg[len], addr, 6);
		len += 6;
	}

	ztest_expect_value(bt_mesh_model_msg_init, opcode,
			   BT_MESH_SILVAIR_ENOCEAN_PROXY_OP);
	ztest_expect_value(bt_mesh_msg_send, model, &mock_model);
	ztest_expect_value(bt_mesh_msg_send, ctx, ctx);
	ztest_expect_value(bt_mesh_msg_send, buf->len, len);
	ztest_expect_data(bt_mesh_msg_send, buf->data, expected_msg);
}

static void mock_button_action(uint8_t changed,
			       enum bt_enocean_button_action action)
{
	zassert_not_null(enocean_cbs->button, "No button callback avilable");
	enocean_cbs->button(&mock_enocean_device, action, changed, NULL, 0);
}

static void mock_timeout(int timer_idx)
{
	zassert_not_null(mock_timers[timer_idx].handler, "No timer handler");
	mock_timers[timer_idx].handler(&mock_timers[timer_idx].dwork->work);
}

static void expect_onoff_set(struct bt_mesh_onoff_cli *cli, bool onoff, uint32_t delay,
			     bool reuse_transaction)
{
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, ctx->addr,
			   srv.buttons[0].onoff.pub.addr);
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, ctx->send_ttl,
			   srv.buttons[0].onoff.pub.ttl);
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, ctx->app_idx,
			   srv.buttons[0].onoff.pub.key);
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, cli, cli);
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, set->reuse_transaction, reuse_transaction);
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, set->transition->time, 0);
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, set->on_off, onoff);
	ztest_expect_value(bt_mesh_onoff_cli_set_unack, set->transition->delay, delay);


}

static void check_phase_a(bool onoff)
{
	k_timeout_t tick_timeout = K_MSEC(50);

	/** Only check i == 1 and up, i == 0 checked in release call. */
	for (int i = 1; i < 4; i++) {
		expect_onoff_set(&srv.buttons[0].onoff, onoff, 150 - (50 * i), true);
		if (i < 3) {
			ztest_expect_value(k_work_reschedule_for_queue, dwork,
						mock_timers[0].dwork);
			ztest_expect_data(k_work_reschedule_for_queue, &delay, &tick_timeout);
		}
		mock_timeout(0);
	}
}

static void expect_delta_set(struct bt_mesh_lvl_cli *lvl, bool new_transaction, int32_t delta,
			     uint32_t delay)
{
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, ctx->addr, lvl->pub.addr);
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, ctx->send_ttl, lvl->pub.ttl);
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, ctx->app_idx, lvl->pub.key);
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, cli, lvl);
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->new_transaction,
			   new_transaction);
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->transition->time,
			   LEVEL_TRANSITION_TIME_MS);
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->delta, delta);
	ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->transition->delay, delay);
}

static void check_phase_b_c(int start, int ticks, bool onoff, bool is_released)
{
	k_timeout_t tick_timeout;

	for (int i = start; i < ticks && i < MAX_TICKS; i++) {
		if (i < 4) {
			/** Phase B */
			expect_delta_set(&srv.buttons[0].lvl, i == 0,
					 (onoff ? 1 : -1) * LEVEL_DELTA,
					 150 - (50 * i));
		} else {
			/** Phase C */
			expect_delta_set(&srv.buttons[0].lvl, i == 0,
					 (i - 2) * (onoff ? 1 : -1) * LEVEL_DELTA,
					 0);
		}

		if (i < (MAX_TICKS - 1)) {
			tick_timeout = (i < 3 || (i == (ticks - 1) && is_released)) ? K_MSEC(50) :
				       K_MSEC(100);
			ztest_expect_value(k_work_reschedule_for_queue, dwork,
					   mock_timers[0].dwork);
			ztest_expect_data(k_work_reschedule_for_queue, &delay, &tick_timeout);
		}

		delta_timedout = i >= MAX_TICKS;

		mock_timeout(0);
	}
}

static void check_phase_d(int32_t delta, int ticks)
{
	k_timeout_t tick_timeout = K_MSEC(50);

	for (int i = 0; i < ticks; i++) {
		expect_delta_set(&srv.buttons[0].lvl, false, delta, 0);

		if (i < (ticks - 1)) {
			ztest_expect_value(k_work_reschedule_for_queue, dwork,
					   mock_timers[0].dwork);
			ztest_expect_data(k_work_reschedule_for_queue, &delay, &tick_timeout);
		}

		mock_timeout(0);
	}
}

static void check_release(bool onoff_released, bool onoff_expected, bool expect_onoff, int wait)
{
	k_timeout_t tick_timeout = K_MSEC(50);

	if (expect_onoff) {
		expect_onoff_set(&srv.buttons[0].onoff, onoff_expected, 150, false);
		ztest_expect_value(k_work_reschedule_for_queue, dwork, mock_timers[0].dwork);
		ztest_expect_data(k_work_reschedule_for_queue, &delay, &tick_timeout);
	}

	mock_button_action(onoff_released ? BT_ENOCEAN_SWITCH_OB : BT_ENOCEAN_SWITCH_IB,
			   BT_ENOCEAN_BUTTON_RELEASE);

	if (expect_onoff) {
		check_phase_a(onoff_expected);
	} else {
		check_phase_b_c(wait, MAX(4, wait + 1), onoff_expected, true);
		check_phase_d(MAX(wait - 2, 1) * (onoff_expected ? 1 : -1) * LEVEL_DELTA, 4);
	}
}

static void check_press(bool onoff)
{
	k_timeout_t wait_timeout = K_MSEC(WAIT_TIME_MS);

	ztest_expect_value(k_work_reschedule_for_queue, dwork, mock_timers[0].dwork);
	ztest_expect_data(k_work_reschedule_for_queue, &delay, &wait_timeout);
	mock_button_action(onoff ? BT_ENOCEAN_SWITCH_OB : BT_ENOCEAN_SWITCH_IB,
			   BT_ENOCEAN_BUTTON_PRESS);
}

static void check_state_machine_vector(bool press1, int wait1,
				       const bool *press2, int wait2,
				       bool release)
{
	check_press(press1);
	check_phase_b_c(0, wait1, press1, false);
	bool expect_onoff_set = wait1 == 0 || wait1 >= MAX_TICKS;

	if (press2) {
		/** Check for dimming to `press1`, FSM will ignore extra presses until release */
		mock_button_action(*press2 ? BT_ENOCEAN_SWITCH_OB : BT_ENOCEAN_SWITCH_IB,
				   BT_ENOCEAN_BUTTON_PRESS);
		check_phase_b_c(wait1, wait1 + wait2, press1, false);
		expect_onoff_set = wait1 + wait2 == 0 || wait1 + wait2 >= MAX_TICKS;
	}
	check_release(release, press1, expect_onoff_set, press2 ? wait1 + wait2 : wait1);
}


static void check_button_sequence(bool press1, bool *press2, bool release)
{
	int wait_times[] = { 0, 1, 10, MAX_TICKS + 1 };

	for (int i = 0; i < ARRAY_SIZE(wait_times); i++) {
		if (!press2) {
			check_state_machine_vector(press1, wait_times[i],
						   NULL, 0, release);
			continue;
		}
		for (int j = 0; j < ARRAY_SIZE(wait_times); j++) {
			check_state_machine_vector(
				press1, wait_times[i], press2,
				wait_times[j], release);
		}
	}
}

ZTEST(silvair_enocean_state_machine_ts, test_state_machine)
{
	bt_addr_le_copy(&srv.addr, &mock_enocean_device.addr);

	for (int first = 0; first < 2; first++) {
		for (int second = -1; second < 2; second++) {
			bool *second_p = second < 0 ? NULL : (bool *)&second;

			for (int release = 0; release < 2; release++) {
				check_button_sequence(first, second_p, release);
			}
		}
	}
}

ZTEST(silvair_enocean_auto_commission_ts, test_auto_commission)
{
	zassert_true(commisioning_enabled, "Commissioning not enabled");
	zassert_not_null(enocean_cbs->commissioned, "Commisioned cb is null");
	zassert_not_null(enocean_cbs->decommissioned, "Decommisioned cb is null");
	zassert_equal(bt_addr_le_cmp(&srv.addr, BT_ADDR_LE_NONE), 0,
		      "Enocean address is not empty");

	/* New device */
	expect_status(NULL, BT_MESH_SILVAIR_ENOCEAN_STATUS_SET,
		      mock_enocean_device.addr.a.val);
	ztest_expect_data(bt_mesh_model_data_store, data, &mock_enocean_device.addr);
	ztest_expect_value(bt_mesh_model_data_store, data_len, sizeof(srv.addr));
	enocean_cbs->commissioned(&mock_enocean_device);
	zassert_equal(bt_addr_le_cmp(&srv.addr, &mock_enocean_device.addr), 0,
		      "Addr not stored with model");
	zassert_true(commisioning_enabled, "Commissioning not enabled");

	/* Remove device */
	expect_status(NULL, BT_MESH_SILVAIR_ENOCEAN_STATUS_SET,
		      BT_ADDR_LE_NONE->a.val);
	ztest_expect_data(bt_mesh_model_data_store, data, NULL);
	ztest_expect_value(bt_mesh_model_data_store, data_len, 0);
	enocean_cbs->decommissioned(&mock_enocean_device);
	zassert_equal(bt_addr_le_cmp(&srv.addr, BT_ADDR_LE_NONE), 0,
		      "Addr not removed with model");
	zassert_true(commisioning_enabled, "Commissioning not enabled");

	/* Same device */
	expect_status(NULL, BT_MESH_SILVAIR_ENOCEAN_STATUS_SET,
		      mock_enocean_device.addr.a.val);
	ztest_expect_data(bt_mesh_model_data_store, data, &mock_enocean_device.addr);
	ztest_expect_value(bt_mesh_model_data_store, data_len, sizeof(srv.addr));
	enocean_cbs->commissioned(&mock_enocean_device);
	zassert_equal(bt_addr_le_cmp(&srv.addr, &mock_enocean_device.addr), 0,
		      "Addr not stored with model");
	zassert_true(commisioning_enabled, "Commissioning not enabled");

	/* Same device, should be no-op */
	enocean_cbs->commissioned(&mock_enocean_device);
	zassert_equal(bt_addr_le_cmp(&srv.addr, &mock_enocean_device.addr), 0,
		      "Addr not stored with model");
	zassert_true(commisioning_enabled, "Commissioning not enabled");

	/* New device, should not be stored and disable further commissioning */
	bt_addr_le_t other_address = {
		.type = BT_ADDR_LE_RANDOM,
		.a = { .val = { 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10, } }
	};
	bt_addr_le_copy(&srv.addr, &other_address);
	ztest_expect_value(bt_enocean_decommission, device, &mock_enocean_device);
	enocean_cbs->commissioned(&mock_enocean_device);
	zassert_false(commisioning_enabled, "Commissioning still enabled");
	zassert_equal(bt_addr_le_cmp(&srv.addr, &other_address), 0,
		      "Existing addr overwritten");
}

ZTEST(silvair_enocean_storage_ts, test_storage)
{
	int err;

	zassert_not_null(_bt_mesh_silvair_enocean_srv_cb.init,
			 "Init cb is null");
	zassert_not_null(_bt_mesh_silvair_enocean_srv_cb.start,
			 "Start cb is null");
	zassert_not_null(_bt_mesh_silvair_enocean_srv_cb.settings_set,
			 "Settings cb is null");
	_bt_mesh_silvair_enocean_srv_cb.init(&mock_model);

	/* Failed load on name supplied */
	err = _bt_mesh_silvair_enocean_srv_cb.settings_set(
		&mock_model, "name", sizeof(stored_addr), mock_read_cb,
		settings_cb_arg);
	zassert_equal(err, -ENOENT, "Expected error not returned");

	/* Failed load on incorrect length */
	ztest_expect_value(mock_read_cb, len, sizeof(stored_addr));
	ztest_expect_value(mock_read_cb, data, &srv.addr);
	ztest_returns_value(mock_read_cb, sizeof(stored_addr) - 1);
	err = _bt_mesh_silvair_enocean_srv_cb.settings_set(
		&mock_model, NULL, sizeof(stored_addr), mock_read_cb,
		settings_cb_arg);
	zassert_equal(err, -EINVAL, "Expected error not returned");

	/* Successful load */
	ztest_expect_value(mock_read_cb, len, sizeof(stored_addr));
	ztest_expect_value(mock_read_cb, data, &srv.addr);
	ztest_returns_value(mock_read_cb, sizeof(stored_addr));
	err = _bt_mesh_silvair_enocean_srv_cb.settings_set(
		&mock_model, NULL, sizeof(stored_addr), mock_read_cb,
		settings_cb_arg);
	zassert_equal(err, 0, "Unexpected error returned");
	_bt_mesh_silvair_enocean_srv_cb.start(&mock_model);
	zassert_equal(bt_addr_le_cmp(&srv.addr, &stored_addr), 0,
		      "Address not loaded");

	/* EnOcean loaded cb unknown device */
	ztest_expect_value(bt_enocean_decommission, device, &mock_enocean_device);
	enocean_cbs->loaded(&mock_enocean_device);
	zassert_equal(bt_addr_le_cmp(&srv.addr, &stored_addr), 0,
		      "Address has changed");

	/* EnOcean loaded cb known device */
	struct bt_enocean_device known_device;

	bt_addr_le_copy(&known_device.addr, &stored_addr);
	enocean_cbs->loaded(&known_device);
	zassert_equal(bt_addr_le_cmp(&srv.addr, &stored_addr), 0,
		      "Address has changed");
}

#define OP_GET 0x0
#define OP_SET 0x1
#define OP_DELETE 0x2

ZTEST(silvair_enocean_messages_ts, test_messages)
{
	static uint8_t key[] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	};
	struct bt_mesh_msg_ctx *ctx = (struct bt_mesh_msg_ctx *)0xABBA;

	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_SILVAIR_ENOCEAN_PROXY_MSG_MAXLEN);
	net_buf_simple_init(&buf, 0);

	/* Test get before set */
	net_buf_simple_add_u8(&buf, OP_GET);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_NOT_SET, NULL);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);

	/* Test set */
	net_buf_simple_reset(&buf);
	net_buf_simple_add_u8(&buf, OP_SET);
	net_buf_simple_add_mem(&buf, key, 16);
	net_buf_simple_add_mem(&buf, mock_enocean_device.addr.a.val, 6);
	ztest_expect_data(bt_enocean_commission, addr, &mock_enocean_device.addr);
	ztest_expect_data(bt_enocean_commission, key, key);
	ztest_returns_value(bt_enocean_commission, 0);
	ztest_expect_value(bt_mesh_model_data_store, data_len,
			   sizeof(mock_enocean_device.addr));
	ztest_expect_data(bt_mesh_model_data_store, data,
			  &mock_enocean_device.addr);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_SET,
		      mock_enocean_device.addr.a.val);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);
	zassert_equal(bt_addr_le_cmp(&mock_enocean_device.addr, &srv.addr), 0,
		      "Address not stored in model");

	/* Test set with same data */
	net_buf_simple_reset(&buf);
	net_buf_simple_add_u8(&buf, OP_SET);
	net_buf_simple_add_mem(&buf, key, 16);
	net_buf_simple_add_mem(&buf, mock_enocean_device.addr.a.val, 6);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_SET,
		      mock_enocean_device.addr.a.val);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);
	zassert_equal(bt_addr_le_cmp(&mock_enocean_device.addr, &srv.addr), 0,
		      "Address not stored in model");

	/* Test set with new data */
	uint8_t other_addr[6] = { 0x62, 0x12, 0x53, 0x45, 0x16, 0x62};

	net_buf_simple_reset(&buf);
	net_buf_simple_add_u8(&buf, OP_SET);
	net_buf_simple_add_mem(&buf, key, 16);
	net_buf_simple_add_mem(&buf, other_addr, 6);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_UNSPECIFIED_ERROR,
		      NULL);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);
	zassert_equal(bt_addr_le_cmp(&mock_enocean_device.addr, &srv.addr), 0,
		      "Address not stored in model");

	/* Test get after set */
	net_buf_simple_reset(&buf);
	net_buf_simple_add_u8(&buf, OP_GET);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_SET,
		      mock_enocean_device.addr.a.val);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);

	/* Test delete */
	net_buf_simple_reset(&buf);
	net_buf_simple_add_u8(&buf, OP_DELETE);
	ztest_expect_data(bt_enocean_decommission, device, &mock_enocean_device);
	ztest_expect_value(bt_mesh_model_data_store, data_len, 0);
	ztest_expect_data(bt_mesh_model_data_store, data, NULL);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_NOT_SET, NULL);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);
	zassert_equal(bt_addr_le_cmp(&srv.addr, BT_ADDR_LE_NONE), 0,
		      "Address not removed");
	zassert_true(commisioning_enabled, "Commissioning not enabled");

	/* Test get after delete */
	net_buf_simple_reset(&buf);
	net_buf_simple_add_u8(&buf, OP_GET);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_NOT_SET, NULL);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);

	/* Test delete again */
	net_buf_simple_reset(&buf);
	net_buf_simple_add_u8(&buf, OP_DELETE);
	expect_status(ctx, BT_MESH_SILVAIR_ENOCEAN_STATUS_NOT_SET, NULL);
	_bt_mesh_silvair_enocean_srv_op[0].func(&mock_model, ctx, &buf);
	zassert_equal(bt_addr_le_cmp(&srv.addr, BT_ADDR_LE_NONE), 0,
		      "Address not removed");
	zassert_true(commisioning_enabled, "Commissioning not enabled");
}

static void setup(void *f)
{
	zassert_not_null(_bt_mesh_silvair_enocean_srv_cb.init,
			 "Init cb is null");
	zassert_not_null(_bt_mesh_silvair_enocean_srv_cb.start,
			 "Start cb is null");
	_bt_mesh_silvair_enocean_srv_cb.init(&mock_model);
	_bt_mesh_silvair_enocean_srv_cb.start(&mock_model);

	srv.buttons[0].onoff.model = &mock_onoff_model;
	srv.buttons[0].onoff.model->pub->msg = &srv.buttons[0].onoff.pub_buf;
	net_buf_simple_init_with_data(&srv.buttons[0].onoff.pub_buf, srv.buttons[0].onoff.pub_data,
				      sizeof(srv.buttons[0].onoff.pub_data));

	srv.buttons[0].lvl.model = &mock_lvl_model;
	srv.buttons[0].lvl.model->pub->msg = &srv.buttons[0].lvl.pub_buf;
	net_buf_simple_init_with_data(&srv.buttons[0].lvl.pub_buf, srv.buttons[0].lvl.pub_data,
				      sizeof(srv.buttons[0].lvl.pub_data));
}

static void teardown(void *f)
{
	zassert_not_null(_bt_mesh_silvair_enocean_srv_cb.reset,
			 "Reset cb is null");
	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[0].dwork);
	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[1].dwork);
	_bt_mesh_silvair_enocean_srv_cb.reset(&mock_model);
	zassert_false(commisioning_enabled, "Commisioning not disabled");
}

static void teardown_expect_flash_clear(void *f)
{
	ztest_expect_data(bt_mesh_model_data_store, data, NULL);
	ztest_expect_value(bt_mesh_model_data_store, data_len, 0);
	teardown(f);
}

static void teardown_expect_clear_and_decommission(void *f)
{
	ztest_expect_value(bt_enocean_decommission, device, &mock_enocean_device);
	teardown_expect_flash_clear(f);
}

ZTEST_SUITE(silvair_enocean_messages_ts, NULL, NULL, setup, teardown, NULL);
ZTEST_SUITE(silvair_enocean_state_machine_ts, NULL, NULL, setup,
	    teardown_expect_clear_and_decommission, NULL);
ZTEST_SUITE(silvair_enocean_auto_commission_ts, NULL, NULL, setup, teardown_expect_flash_clear,
	    NULL);
ZTEST_SUITE(silvair_enocean_storage_ts, NULL, NULL, NULL, teardown_expect_flash_clear, NULL);
