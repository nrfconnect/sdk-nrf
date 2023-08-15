/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>
#include <bluetooth/mesh/time_srv.h>
#include <bluetooth/mesh/models.h>
#include "time_internal.h"
#include "time_util.h"

#define UNCERTAINTY_STEP_MS 10
#define SUBSEC_STEPS        256U
#define STATUS_INTERVAL_MIN 30000ul
#define TEST_TIME           100000ul

static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);

static struct bt_mesh_elem mock_elem[] = {
	BT_MESH_ELEM(1, BT_MESH_MODEL_LIST(BT_MESH_MODEL_TIME_SRV(&time_srv)), BT_MESH_MODEL_NONE)};

struct mock_comp_data {
	size_t elem_count;
	struct bt_mesh_elem *elem;
} mock_comp = {.elem_count = 1, .elem = mock_elem};

static struct bt_mesh_msg_ctx test_ctx = {
	.addr = 0x0001,
	.send_ttl = 0,
};

static const struct bt_mesh_time_zone_change zone_change = {.new_offset = 3, .timestamp = 30};

static const struct bt_mesh_time_tai_utc_change tai_utc_change = {.delta_new = 4, .timestamp = 40};

static struct bt_mesh_time_status expected_status;
static uint32_t time_status_rx_number;
static bool is_randomized;
static bool is_unsolicited;

static void tc_setup(void *f)
{
	struct bt_mesh_time_tai tai;
	struct tm start_tm = {
		/* 1st of Jan 2010 */
		.tm_year = 110, .tm_mon = 0, .tm_mday = 1, .tm_wday = 5,
		.tm_hour = 0,   .tm_min = 0, .tm_sec = 0,
	};

	time_status_rx_number = 0;
	zassert_not_null(_bt_mesh_time_srv_cb.init, "Init cb is null");
	_bt_mesh_time_srv_cb.init(mock_elem[0].models);

	bt_mesh_time_srv_time_zone_change_set(&time_srv, &zone_change);
	bt_mesh_time_srv_tai_utc_change_set(&time_srv, &tai_utc_change);
	bt_mesh_time_srv_role_set(&time_srv, BT_MESH_TIME_RELAY);

	zassert_ok(ts_to_tai(&tai, &start_tm), "cannot convert tai time");
	time_srv.data.sync.uptime = k_uptime_get();
	time_srv.data.sync.status.tai = tai;
}

static void tc_teardown(void *f)
{
	zassert_not_null(_bt_mesh_time_srv_cb.reset, "Reset cb is null");
	_bt_mesh_time_srv_cb.reset(mock_elem[0].models);
}

int bt_mesh_model_extend(struct bt_mesh_model *extending_mod, struct bt_mesh_model *base_mod)
{
	return 0;
}

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
}

int bt_mesh_msg_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	struct bt_mesh_time_status status;
	uint64_t expected_uncertity =
		(expected_status.uncertainty + CONFIG_BT_MESH_TIME_MESH_HOP_UNCERTAINTY) /
		UNCERTAINTY_STEP_MS;
	int64_t steps =
		(SUBSEC_STEPS * (k_uptime_get() - time_srv.data.sync.uptime)) / MSEC_PER_SEC;

	zassert_equal_ptr(time_srv.model, model);

	if (is_randomized) {
		zassert_is_null(ctx);
	} else {
		if (is_unsolicited) {
			zassert_is_null(ctx);
		} else {
			zassert_not_null(ctx);
		}
		is_unsolicited = !is_unsolicited;
	}

	bt_mesh_time_decode_time_params(buf, &status);

	zassert_equal(status.tai.sec, expected_status.tai.sec);
	zassert_equal(status.tai.subsec, expected_status.tai.subsec + steps);
	zassert_equal(status.tai_utc_delta, expected_status.tai_utc_delta);
	zassert_equal(status.is_authority, expected_status.is_authority);
	zassert_equal(status.time_zone_offset, expected_status.time_zone_offset);

	if (is_randomized) {
		zassert_between_inclusive(status.uncertainty,
					  expected_uncertity * UNCERTAINTY_STEP_MS + 20,
					  expected_uncertity * UNCERTAINTY_STEP_MS + 50);
	} else {
		zassert_equal(status.uncertainty, expected_uncertity * UNCERTAINTY_STEP_MS);
	}

	time_status_rx_number++;

	return 0;
}

int bt_mesh_model_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *msg, const struct bt_mesh_send_cb *cb, void *cb_data)
{
	return 0;
}

int bt_rand(void *buf, size_t len)
{
	sys_rand_get(buf, len);
	return 0;
}

static void rx_time_status(void)
{
	/* Create a time status */
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_STATUS,
				 BT_MESH_TIME_MSG_MAXLEN_TIME_STATUS);
	bt_mesh_time_srv_status(&time_srv, k_uptime_get(), &expected_status);
	bt_mesh_time_encode_time_params(&msg, &expected_status);

	_bt_mesh_time_srv_op[1].func(time_srv.model, &test_ctx, &msg);

	k_sleep(K_SECONDS(1));
}

static void rx_time_set(void)
{
	/* Create a time set */
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_TIME_OP_TIME_SET, BT_MESH_TIME_MSG_LEN_TIME_SET);
	bt_mesh_time_srv_status(&time_srv, k_uptime_get(), &expected_status);
	bt_mesh_time_encode_time_params(&msg, &expected_status);

	_bt_mesh_time_setup_srv_op[0].func(time_srv.model, &test_ctx, &msg);

	k_sleep(K_SECONDS(1));
}

/** Test scenario: Time Relay receives Time Status every second
 *  during 100 seconds. Expectation: device will relay only 4 Time Statuses
 *  with randomization.
 */
ZTEST(time_model, test_time_status_timing)
{
	int64_t start_time = k_uptime_get();

	is_randomized = true;

	while (k_uptime_get() - start_time < TEST_TIME) {
		rx_time_status();
	}

	zassert_equal(TEST_TIME / STATUS_INTERVAL_MIN + 1, time_status_rx_number);
}

/** Test scenario: Time Relay receives Time Set every second
 *  during 100 seconds. Expectation: device will publish Time Status with publication settings
 *  and response Time Status to sender every received Time Set message without randomization.
 */
ZTEST(time_model, test_time_set_timing)
{
	int64_t start_time = k_uptime_get();

	is_randomized = false;
	is_unsolicited = true;

	while (k_uptime_get() - start_time < TEST_TIME) {
		rx_time_set();
	}

	zassert_equal(2 * TEST_TIME / 1000, time_status_rx_number);
}

/** Test scenario: Time Relay receives Time Status once. Then it receives Time Set and then
 *  it receives Time Status very second during 100 seconds.
 *  Expectation: previously received Time Status doesn't impact on Time Set replies both
 *  publishing and response. However, Time Set published reply triggers 30 seconds delay
 *  for next Time Status handler (Time Relay relays only 3 Time Statuses instead of 4).
 */
ZTEST(time_model, test_time_set_status_mix)
{
	is_randomized = true;

	rx_time_status();
	zassert_equal(1, time_status_rx_number);

	is_randomized = false;
	is_unsolicited = true;
	time_status_rx_number = 0;

	rx_time_set();
	zassert_equal(2, time_status_rx_number);

	int64_t start_time = k_uptime_get();

	is_randomized = true;
	time_status_rx_number = 0;

	while (k_uptime_get() - start_time < TEST_TIME) {
		rx_time_status();
	}

	zassert_equal(TEST_TIME / STATUS_INTERVAL_MIN, time_status_rx_number);
}

/** Test scenario: Time Relay publishes Time Status over periodic publishing handler.
 *  Then it receives Time Status every second during 100 seconds.
 *  Expectation: preiodic publishing triggers 30 seconds delay and device relays
 *  3 statuses instead of 4.
 */
ZTEST(time_model, test_time_periodic_pub_status_mix)
{
	zassert_ok(_bt_mesh_time_srv_update_handler(mock_elem[0].models));

	int64_t start_time = k_uptime_get();

	is_randomized = true;

	while (k_uptime_get() - start_time < TEST_TIME) {
		rx_time_status();
	}

	zassert_equal(TEST_TIME / STATUS_INTERVAL_MIN, time_status_rx_number);
}

ZTEST_SUITE(time_model, NULL, NULL, tc_setup, tc_teardown, NULL);
