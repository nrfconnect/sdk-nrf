/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ztest.h>
#include <bluetooth/enocean.h>
#include <bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/vnd/silvair_enocean_srv.h>

#define MAX_TICKS (CONFIG_BT_MESH_SILVAIR_ENOCEAN_DELTA_TIMEOUT / \
		  CONFIG_BT_MESH_SILVAIR_ENOCEAN_TICK_TIME)

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

static struct bt_mesh_model mock_model = {
	.user_data = &srv
};

static struct bt_mesh_model mock_lvl_model = {
	.pub = &srv.buttons[0].lvl.pub,
	.user_data = &srv.buttons[0].lvl,
};

static struct bt_mesh_model mock_onoff_model = {
	.pub = &srv.buttons[0].onoff.pub,
	.user_data = &srv.buttons[0].onoff
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

int bt_enocean_commission(const bt_addr_le_t *addr, const uint8_t *key,
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

struct bt_mesh_elem *bt_mesh_model_elem(struct bt_mesh_model *mod)
{
	return NULL;
}

struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
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
	zassert_not_null(set->transition, "transition is null");
	ztest_check_expected_value(set->transition->time);
	ztest_check_expected_value(set->transition->delay);
	zassert_is_null(ctx, "Context was not null");
	ztest_check_expected_value(cli->pub.retransmit);
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
	zassert_is_null(ctx, "Context was not null");
	ztest_check_expected_value(cli->pub.retransmit);
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

int bt_mesh_model_data_store(struct bt_mesh_model *model, bool vnd,
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

int model_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
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
			  uint8_t *addr)
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
	ztest_expect_value(model_send, model, &mock_model);
	ztest_expect_value(model_send, ctx, ctx);
	ztest_expect_value(model_send, buf->len, len);
	ztest_expect_data(model_send, buf->data, expected_msg);
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

static void check_release(bool onoff, bool expect_generic_set, int wait)
{
	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[0].dwork);
	if (expect_generic_set) {
		ztest_expect_value(bt_mesh_onoff_cli_set_unack, cli, &srv.buttons[0].onoff);
		ztest_expect_value(bt_mesh_onoff_cli_set_unack, set->on_off, onoff);
		ztest_expect_value(bt_mesh_onoff_cli_set_unack, set->transition->time, 0);
		ztest_expect_value(bt_mesh_onoff_cli_set_unack, set->transition->delay, 150);
		ztest_expect_value(bt_mesh_onoff_cli_set_unack, cli->pub.retransmit,
				   BT_MESH_TRANSMIT(3, 50));
	} else {
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, cli, &srv.buttons[0].lvl);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->delta, (wait + 1) *
				   (onoff ? 1 : -1) * CONFIG_BT_MESH_SILVAIR_ENOCEAN_DELTA);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->new_transaction,
				   false);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->transition->time,
				   CONFIG_BT_MESH_SILVAIR_ENOCEAN_DELTA_TRANSITION_TIME);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack,
				   delta_set->transition->delay, 0);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, cli->pub.retransmit,
				   BT_MESH_TRANSMIT(4, 50));
	}

	mock_button_action(onoff ? BT_ENOCEAN_SWITCH_OB : BT_ENOCEAN_SWITCH_IB,
			   BT_ENOCEAN_BUTTON_RELEASE);

	if (expect_generic_set) {
		srv.buttons[0].onoff.pub_buf.len = 6;
		srv.buttons[0].onoff.pub.count = 2;
		srv.buttons[0].onoff.pub.update(srv.buttons[0].onoff.model);
		zassert_equal(srv.buttons[0].onoff.pub_buf.data[5], model_delay_encode(100),
			      "Encoded incorrect delay");
	} else {
		srv.buttons[0].lvl.pub_buf.len = 9;
		srv.buttons[0].lvl.pub.count = 1;
		srv.buttons[0].lvl.pub.update(srv.buttons[0].lvl.model);
		zassert_equal(srv.buttons[0].lvl.pub_buf.data[8], 0, "Encoded incorrect delay");
	}
}

static void check_press(bool onoff)
{
	k_timeout_t wait_timeout = K_MSEC(CONFIG_BT_MESH_SILVAIR_ENOCEAN_WAIT_TIME);

	ztest_expect_value(k_work_reschedule_for_queue, dwork, mock_timers[0].dwork);
	ztest_expect_data(k_work_reschedule_for_queue, &delay, &wait_timeout);
	mock_button_action(onoff ? BT_ENOCEAN_SWITCH_OB : BT_ENOCEAN_SWITCH_IB,
			   BT_ENOCEAN_BUTTON_PRESS);
}

static void check_ticks(bool onoff, int tick_count)
{
	k_timeout_t tick_timeout = K_MSEC(CONFIG_BT_MESH_SILVAIR_ENOCEAN_TICK_TIME);

	for (int i = 0; i < tick_count && i <= MAX_TICKS; i++) {
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, cli, &srv.buttons[0].lvl);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->delta, (i + 1) *
				   (onoff ? 1 : -1) * CONFIG_BT_MESH_SILVAIR_ENOCEAN_DELTA);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->new_transaction,
				   i == 0);
		ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, delta_set->transition->time,
				   CONFIG_BT_MESH_SILVAIR_ENOCEAN_DELTA_TRANSITION_TIME);
		if (i == 0) {
			ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack,
					   delta_set->transition->delay, 150);
			ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, cli->pub.retransmit,
					   BT_MESH_TRANSMIT(3, 50));
		} else {
			ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack,
					   delta_set->transition->delay, 0);
			ztest_expect_value(bt_mesh_lvl_cli_delta_set_unack, cli->pub.retransmit, 0);
		}
		if (i < MAX_TICKS) {
			ztest_expect_value(k_work_reschedule_for_queue, dwork,
					   mock_timers[0].dwork);
			ztest_expect_data(k_work_reschedule_for_queue, &delay,
					  &tick_timeout);
		}

		delta_timedout = i >= MAX_TICKS;

		mock_timeout(0);

		if (i == 0) {
			srv.buttons[0].lvl.pub_buf.len = 9;
			srv.buttons[0].lvl.pub.count = 0;
			srv.buttons[0].lvl.pub.update(srv.buttons[0].lvl.model);
			zassert_equal(srv.buttons[0].lvl.pub_buf.data[8], 0,
				      "Encoded incorrect delay");
		}
	}
}

static void check_state_machine_vector(bool press1, int wait1,
				       const bool *press2, int wait2,
				       bool release)
{
	check_press(press1);
	check_ticks(press1, wait1);
	bool expect_generic_set = wait1 == 0 || wait1 >= MAX_TICKS || press1 != release;

	if (press2) {
		check_press(*press2);
		check_ticks(*press2, wait2);
		expect_generic_set = wait2 == 0 || wait2 >= MAX_TICKS || *press2 != release;
	}
	check_release(release, expect_generic_set, press2 ? wait2 : wait1);
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

static void test_state_machine(void)
{
	bt_addr_le_copy(&srv.addr, &mock_enocean_device.addr);

	/*
	 * Test release from IDLE state
	 */
	check_release(true, true, 0);
	check_release(false, true, 0);


	for (int first = 0; first < 2; first++) {
		for (int second = -1; second < 2; second++) {
			bool *second_p = second < 0 ? NULL : (bool *)&second;

			for (int release = 0; release < 2; release++) {
				check_button_sequence(first, second_p, release);
			}
		}
	}
}

static void test_auto_commission(void)
{
	zassert_true(commisioning_enabled, "Commissioning not enabled");
	zassert_not_null(enocean_cbs->commissioned, "Commisioned cb is null");
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

static void test_storage(void)
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

static void test_messages(void)
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

static void setup(void)
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

static void teardown(void)
{
	zassert_not_null(_bt_mesh_silvair_enocean_srv_cb.reset,
			 "Reset cb is null");
	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[0].dwork);
	ztest_expect_value(k_work_cancel_delayable, dwork, mock_timers[1].dwork);
	_bt_mesh_silvair_enocean_srv_cb.reset(&mock_model);
	zassert_false(commisioning_enabled, "Commisioning not disabled");
}

static void teardown_expect_flash_clear(void)
{
	ztest_expect_data(bt_mesh_model_data_store, data, NULL);
	ztest_expect_value(bt_mesh_model_data_store, data_len, 0);
	teardown();
}

static void teardown_expect_clear_and_decommission(void)
{
	ztest_expect_value(bt_enocean_decommission, device, &mock_enocean_device);
	teardown_expect_flash_clear();
}

void test_main(void)
{
	ztest_test_suite(
		silvair_enocean_test,
		ztest_unit_test_setup_teardown(test_messages,
					       setup, teardown),
		ztest_unit_test_setup_teardown(
			test_state_machine,
			setup,
			teardown_expect_clear_and_decommission),
		ztest_unit_test_setup_teardown(test_auto_commission,
					       setup,
					       teardown_expect_flash_clear),
		ztest_unit_test_setup_teardown(test_storage,
					       unit_test_noop,
					       teardown_expect_flash_clear)
			 );

	ztest_run_test_suite(silvair_enocean_test);
}
