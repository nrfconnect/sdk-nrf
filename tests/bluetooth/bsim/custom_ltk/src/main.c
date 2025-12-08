/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

#include "bs_tracing.h"
#include "bs_types.h"
#include "bstests.h"
#include "time_machine.h"

#include <zephyr/sys/__assert.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

#include <bluetooth/nrf/host_extensions.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

DEFINE_FLAG(flag_is_connected);
DEFINE_FLAG(flag_sec_lvl_changed);

static struct bt_conn *test_conn;
static struct bt_nrf_ltk test_ltk = {
	{0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf},};

typedef void (*conn_action_cb)(void);

struct test_params {
	conn_action_cb conn_cb;
	bt_security_t sec_lvl;
	bool exp_err;
	bool auth;
};

static struct test_params *p_test;
static bool is_bondable;

static void set_ltk(void)
{
	int err;

	LOG_INF("Set custom LTK");
	err = bt_nrf_conn_set_ltk(test_conn, &test_ltk, p_test->auth);
	TEST_ASSERT(!err, "bt_nrf_conn_set_ltk failed (%d).", err);
}

static void set_wrong_ltk(void)
{
	test_ltk.val[0] = !test_ltk.val[0];
	set_ltk();
}

static void set_bondable(void)
{
	int err;

	is_bondable = true;
	err = bt_conn_set_bondable(test_conn, is_bondable);
	TEST_ASSERT(!err, "bt_conn_set_bondable failed (%d).", err);
}

static void clear_conn(void)
{
	if (test_conn) {
		bt_conn_unref(test_conn);
		test_conn = NULL;
	}
}

static void check_sec_info(struct bt_conn *conn)
{
	int err;
	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	TEST_ASSERT(!err, "bt_conn_get_info failed (%d).", err);

	TEST_ASSERT(info.security.level == p_test->sec_lvl, "Got sec level %u, expected %u",
				info.security.level, p_test->sec_lvl);
	TEST_ASSERT(info.security.enc_key_size == sizeof(test_ltk.val), "Key size didn't match");
	TEST_ASSERT(info.security.flags == BT_SECURITY_FLAG_SC, "Sec flags didn't match");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT((!test_conn || (conn == test_conn)), "Unexpected new connection.");

	if (!test_conn) {
		test_conn = bt_conn_ref(conn);
	}

	if (err != 0) {
		clear_conn();
		TEST_ASSERT(conn, "Connection attempt failed with %d", err);
		return;
	}

	LOG_INF("Connected");

	if (p_test->conn_cb) {
		p_test->conn_cb();
	}

	SET_FLAG(flag_is_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected");
	UNSET_FLAG(flag_is_connected);
}

static void sec_changed(struct bt_conn *conn, bt_security_t level,
					enum bt_security_err err)
{
	if (p_test->exp_err) {
		TEST_ASSERT(err, "Security update expected to fail.");
	} else {
		TEST_ASSERT(!err, "Security level update failed (%u).", err);

		LOG_INF("Sec level set %u", level);
		check_sec_info(conn);
		SET_FLAG(flag_sec_lvl_changed);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = sec_changed,
};

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	if (!p_test->exp_err) {
		TEST_FAIL("Pairing failed (unexpected): reason %u", reason);
	}
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	TEST_ASSERT(is_bondable == bonded,
				"Expected bonding status: %u, got %u", is_bondable, bonded);
}

static struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {
	.pairing_failed = pairing_failed,
	.pairing_complete = pairing_complete,
};

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi,
				uint8_t type, struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (test_conn != NULL) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_HCI_ADV_IND && type != BT_HCI_ADV_DIRECT_IND) {
		TEST_FAIL("Unexpected advertisement type.");
	}

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	err = bt_le_scan_stop();
	TEST_ASSERT(!err, "Err bt_le_scan_stop %d", err);

	err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &test_conn);
	TEST_ASSERT(!err, "Err bt_conn_le_create %d", err);
}

static void scan_and_connect(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_cb);
	TEST_ASSERT(!err, "Err bt_le_scan_start %d", err);

	WAIT_FOR_FLAG(flag_is_connected);
}

static void disconnect_and_clear(void)
{
	int err;
	struct bt_conn_info info;

	err = bt_conn_get_info(test_conn, &info);
	TEST_ASSERT(!err, "bt_conn_get_info failed (%d).", err);
	TEST_ASSERT(info.state == BT_CONN_STATE_CONNECTED, "Not connected");

	if (info.role == BT_CONN_ROLE_CENTRAL) {
		err = bt_conn_disconnect(test_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		TEST_ASSERT(!err, "Err bt_conn_disconnect %d", err);
	}

	WAIT_FOR_FLAG_UNSET(flag_is_connected);
	clear_conn();
}

static void advertise_and_connect(void)
{
	int err;
	struct bt_le_adv_param param = {};

	param.id = BT_ID_DEFAULT;
	param.interval_min = 0x0020;
	param.interval_max = 0x4000;
	param.options |= BT_LE_ADV_OPT_CONN;

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)", err);

	WAIT_FOR_FLAG(flag_is_connected);
}

static void update_security(bt_security_t sec)
{
	int err;

	err = bt_conn_set_security(test_conn, sec);
	TEST_ASSERT(!err, "Err bt_conn_update_security %d", err);

	if (!p_test->exp_err) {
		TAKE_FLAG(flag_sec_lvl_changed);
	}
}

static void wait_security_update(void)
{
	TAKE_FLAG(flag_sec_lvl_changed);
}

static void unpair(void)
{
	int err;

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	TEST_ASSERT(!err, "Failed bt_unpair %d", err);
}

static void passkey_confirm_cb(struct bt_conn *conn, unsigned int passkey)
{
	int err;

	if (p_test->conn_cb == set_ltk) {
		TEST_FAIL("Passkey confirm not expected when custom LTK is used");
	}

	err = bt_conn_auth_passkey_confirm(conn);
	TEST_ASSERT(!err, "Failed bt_conn_auth_passkey_confirm %d", err);
}

static void cancel_cb(struct bt_conn *conn)
{
	TEST_FAIL("Unexpected authentication canceled");
}

static void passkey_display_cb(struct bt_conn *conn, unsigned int passkey)
{
	LOG_INF("Passkey: %u", passkey);
}

static struct bt_conn_auth_cb auth_cb = {
	.passkey_display = passkey_display_cb,
	.passkey_confirm = passkey_confirm_cb,
	.cancel = cancel_cb,
};

static void enable_passkey(void)
{
	int err;

	err = bt_conn_auth_cb_register(&auth_cb);
	TEST_ASSERT(!err, "Failed bt_conn_auth_cb_register %d", err);
}

static void test_setup(void)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(!err, "bt_enable failed.");

	err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);
	TEST_ASSERT(!err, "bt_conn_auth_info_cb_register failed (%d).", err);
}

void central_ltk_test(void)
{
	test_setup();

	struct test_params tests[] = {
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L2, .exp_err = false, .auth = false},
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = true},
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = true},};

	ARRAY_FOR_EACH_PTR(tests, test) {
		p_test = test;
		scan_and_connect();
		update_security(BT_SECURITY_L2);
		disconnect_and_clear();
	}

	TEST_PASS("PASS");
}

void peripheral_ltk_test(void)
{
	test_setup();

	struct test_params tests[] = {
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L2, .exp_err = false, .auth = false},
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = true},
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = true},};

	ARRAY_FOR_EACH_PTR(tests, test) {
		p_test = test;
		advertise_and_connect();
		wait_security_update();
		disconnect_and_clear();
	}

	TEST_PASS("PASS");
}

void central_ltk_coex_test(void)
{
	test_setup();
	enable_passkey();

	is_bondable = bt_get_bondable();
	TEST_ASSERT(!is_bondable, "Bonding flag is already set");

	struct test_params tests[] = {
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = true},
		{.conn_cb = NULL, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = false},
		{.conn_cb = set_bondable, .sec_lvl = BT_SECURITY_L4, .exp_err = false,
			.auth = false},
	};

	ARRAY_FOR_EACH_PTR(tests, test) {
		p_test = test;
		scan_and_connect();
		update_security(test->sec_lvl);
		disconnect_and_clear();
		unpair();
	}

	TEST_PASS("PASS");
}

void peripheral_ltk_coex_test(void)
{
	test_setup();
	enable_passkey();

	is_bondable = bt_get_bondable();
	TEST_ASSERT(!is_bondable, "Bonding flag is already set");

	struct test_params tests[] = {
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = true},
		{.conn_cb = NULL, .sec_lvl = BT_SECURITY_L4, .exp_err = false, .auth = false},
		{.conn_cb = set_bondable, .sec_lvl = BT_SECURITY_L4, .exp_err = false,
			.auth = false},
	};

	ARRAY_FOR_EACH_PTR(tests, test) {
		p_test = test;
		advertise_and_connect();
		wait_security_update();
		disconnect_and_clear();
		unpair();
	}

	TEST_PASS("PASS");
}

void central_invalid_ltk_test(void)
{
	test_setup();

	struct test_params tests[] = {
		{.conn_cb = NULL, .sec_lvl = BT_SECURITY_L2, .exp_err = true, .auth = false},
		{.conn_cb = set_wrong_ltk, .sec_lvl = BT_SECURITY_L2, .exp_err = true,
			.auth = false},
	};

	ARRAY_FOR_EACH_PTR(tests, test) {
		p_test = test;
		scan_and_connect();
		update_security(test->sec_lvl);
		disconnect_and_clear();
	}

	TEST_PASS("PASS");
}

void peripheral_invalid_ltk_test(void)
{
	test_setup();

	struct test_params tests[] = {
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L2, .exp_err = true, .auth = false},
		{.conn_cb = set_ltk, .sec_lvl = BT_SECURITY_L2, .exp_err = true, .auth = false},
	};

	ARRAY_FOR_EACH_PTR(tests, test) {
		p_test = test;
		advertise_and_connect();
		disconnect_and_clear();
	}

	TEST_PASS("PASS");
}

static const struct bst_test_instance test_to_add[] = {
	{
		.test_id = "central_ltk_test",
		.test_main_f = central_ltk_test,
	},
	{
		.test_id = "peripheral_ltk_test",
		.test_main_f = peripheral_ltk_test,
	},
	{
		.test_id = "central_ltk_coex_test",
		.test_main_f = central_ltk_coex_test,
	},
	{
		.test_id = "peripheral_ltk_coex_test",
		.test_main_f = peripheral_ltk_coex_test,
	},
	{
		.test_id = "central_invalid_ltk_test",
		.test_main_f = central_invalid_ltk_test,
	},
	{
		.test_id = "peripheral_invalid_ltk_test",
		.test_main_f = peripheral_invalid_ltk_test,
	},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_to_add);
}

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
