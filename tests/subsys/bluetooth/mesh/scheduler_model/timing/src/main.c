/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
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
#include <scheduler_internal.h>
#include <model_utils.h>
#include <time_util.h>
#include <sched_test.h>

static struct bt_mesh_scene_srv scene_srv;
static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);
static struct bt_mesh_scheduler_srv scheduler_srv =
	BT_MESH_SCHEDULER_SRV_INIT(NULL, &time_srv);

K_SEM_DEFINE(action_fired, 0, 1);

static const struct bt_mesh_model mock_sched_model = {
	.rt = &(struct bt_mesh_model_rt_ctx){.user_data = &scheduler_srv, .elem_idx = 1}
};

static const struct bt_mesh_model mock_scene_model = {
	.rt = &(struct bt_mesh_model_rt_ctx){.user_data = &scene_srv, .elem_idx = 1}
};

static const struct bt_mesh_elem dummy_elem = {
	.rt = &(struct bt_mesh_elem_rt_ctx){.addr = 1}
};

static struct tm start_tm;

static int gfire_cnt;
static struct tm *fired_tm;
static int galloc_cnt;

/* redefined mocks */
uint8_t model_transition_encode(int32_t transition_time)
{
	return 0;
}

int32_t model_transition_decode(uint8_t encoded_transition)
{
	return 0;
}

const struct bt_mesh_elem *bt_mesh_model_elem(const struct bt_mesh_model *mod)
{
	return &dummy_elem;
}

struct bt_mesh_elem *bt_mesh_elem_find(uint16_t addr)
{
	return NULL;
}

const struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
					       uint16_t id)
{
	return &mock_scene_model;
}

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
	net_buf_simple_init(msg, 0);
}

int bt_mesh_scene_srv_set(struct bt_mesh_scene_srv *srv, uint16_t scene,
			  struct bt_mesh_model_transition *transition)
{
	zassert_not_equal(gfire_cnt, 0, "Unexpected fired action");

	gfire_cnt--;

	if (fired_tm) {
		fired_tm[galloc_cnt] = *bt_mesh_time_srv_localtime(&time_srv, k_uptime_get());
		galloc_cnt++;
	}

	if (gfire_cnt == 0) {
		k_sem_give(&action_fired);
	}

	return 0;
}

int bt_mesh_scene_srv_pub(struct bt_mesh_scene_srv *srv,
			 struct bt_mesh_msg_ctx *ctx)
{
	return 0;
}

int bt_mesh_onoff_srv_pub(struct bt_mesh_onoff_srv *srv,
			  struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_onoff_status *status)
{
	return 0;
}

void bt_mesh_time_encode_time_params(struct net_buf_simple *buf,
				     const struct bt_mesh_time_status *status)
{
}

int bt_mesh_msg_send(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	return 0;
}

int _bt_mesh_time_srv_update_handler(const struct bt_mesh_model *model)
{
	return 0;
}

int64_t bt_mesh_time_srv_mktime(struct bt_mesh_time_srv *srv, struct tm *timeptr)
{
	return (timeutil_timegm64(timeptr) - timeutil_timegm64(&start_tm)) * 1000;
}

struct tm *bt_mesh_time_srv_localtime(struct bt_mesh_time_srv *srv,
				      int64_t uptime)
{
	static struct tm timeptr;
	struct bt_mesh_time_tai tai;

	zassert_ok(ts_to_tai(&tai, &start_tm), "cannot convert tai time");

	int64_t tmp =  tai_to_ms(&tai);

	tmp += uptime;
	tai = tai_at(tmp);
	tai_to_ts(&tai, &timeptr);

	return &timeptr;
}
/* redefined mocks */

static void tc_setup(void *f)
{
	/* 1st of Jan 2010 */
	start_tm.tm_year = 110;
	start_tm.tm_mon = 0;
	start_tm.tm_mday = 1;
	start_tm.tm_wday = 5;
	start_tm.tm_hour = 0;
	start_tm.tm_min = 0;
	start_tm.tm_sec = 0;

	gfire_cnt = 0;
	galloc_cnt = 0;
	fired_tm = NULL;

	k_sem_reset(&action_fired);
	zassert_not_null(_bt_mesh_scheduler_srv_cb.init,
			 "Init cb is null");
	_bt_mesh_scheduler_srv_cb.init(&mock_sched_model);
}

static void tc_teardown(void *f)
{
	if (fired_tm) {
		free(fired_tm);
	}
	zassert_not_null(_bt_mesh_scheduler_srv_cb.reset,
			 "Reset cb is null");
	_bt_mesh_scheduler_srv_cb.reset(&mock_sched_model);
}

static void action_put(const struct bt_mesh_schedule_entry *test_action)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);

	scheduler_action_pack(&buf, 0, test_action);

	start_time_adjust(&start_tm);

	zassert_false(_bt_mesh_scheduler_setup_srv_op[1].func(&mock_sched_model, NULL, &buf),
		"Cannot schedule test action.");
}

/* expected time in seconds */
static void measurement_start(int64_t expected_time, int fire_cnt)
{
	gfire_cnt = fire_cnt;

	fired_tm = malloc(sizeof(struct tm) * fire_cnt);
	zassert_not_null(fired_tm, "Cannot allocate memory for test");

	zassert_ok(k_sem_take(&action_fired, K_SECONDS(expected_time + DELTA_TIME)),
		"Scheduled action isn't fired in time.");
}

ZTEST(scheduler_timing, test_exact_time_simple)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = 1,
			.hour = 0,
			.minute = 0,
			.second = 30,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	action_put(&test_action);
	measurement_start(30, 1);

	struct tm expected = {
		TM_INIT(110, 0, 1, 0, 0, 30)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_exact_time_second_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = 1,
			.hour = 10,
			.minute = 1,
			.second = 5,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_hour = 10;
	start_tm.tm_min = 0;
	start_tm.tm_sec = 30;

	action_put(&test_action);
	measurement_start(35, 1);

	struct tm expected = {
		TM_INIT(110, 0, 1, 10, 1, 5)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_every_15_seconds)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = BT_MESH_SCHEDULER_ANY_MINUTE,
			.second = BT_MESH_SCHEDULER_EVERY_15_SECONDS,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	action_put(&test_action);
	measurement_start(60, 4);

	struct tm expected[4] = {
		{TM_INIT(110, 0, 1, 0, 0, 15)},
		{TM_INIT(110, 0, 1, 0, 0, 30)},
		{TM_INIT(110, 0, 1, 0, 0, 45)},
		{TM_INIT(110, 0, 1, 0, 1, 0)}
	};

	expected_tm_check(fired_tm, expected, 4);
}

ZTEST(scheduler_timing, test_every_20_seconds)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = BT_MESH_SCHEDULER_ANY_MINUTE,
			.second = BT_MESH_SCHEDULER_EVERY_20_SECONDS,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	action_put(&test_action);
	measurement_start(60, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 1, 0, 0, 20)},
		{TM_INIT(110, 0, 1, 0, 0, 40)},
		{TM_INIT(110, 0, 1, 0, 1, 0)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_once_a_minute)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = BT_MESH_SCHEDULER_ANY_MINUTE,
			.second = BT_MESH_SCHEDULER_ONCE_A_MINUTE,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_sec = 59;

	action_put(&test_action);
	measurement_start(180, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 1, 0, 1, INT_MAX)},
		{TM_INIT(110, 0, 1, 0, 2, INT_MAX)},
		{TM_INIT(110, 0, 1, 0, 3, INT_MAX)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_any_second)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = BT_MESH_SCHEDULER_ANY_MINUTE,
			.second = BT_MESH_SCHEDULER_ANY_SECOND,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	action_put(&test_action);
	measurement_start(3, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 1, 0, 0, 1)},
		{TM_INIT(110, 0, 1, 0, 0, 2)},
		{TM_INIT(110, 0, 1, 0, 0, 3)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_exact_time_minute_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = 1,
			.hour = 11,
			.minute = 5,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_hour = 10;
	start_tm.tm_min = 6;

	action_put(&test_action);
	measurement_start(59 * 60, 1);

	struct tm expected = {
		TM_INIT(110, 0, 1, 11, 5, 0)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_every_15_minutes)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = BT_MESH_SCHEDULER_ANY_HOUR,
			.minute = BT_MESH_SCHEDULER_EVERY_15_MINUTES,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	action_put(&test_action);
	measurement_start(60 * 60, 4);

	struct tm expected[4] = {
		{TM_INIT(110, 0, 1, 0, 15, 0)},
		{TM_INIT(110, 0, 1, 0, 30, 0)},
		{TM_INIT(110, 0, 1, 0, 45, 0)},
		{TM_INIT(110, 0, 1, 1, 0, 0)}
	};

	expected_tm_check(fired_tm, expected, 4);
}

ZTEST(scheduler_timing, test_every_20_minutes)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = BT_MESH_SCHEDULER_ANY_HOUR,
			.minute = BT_MESH_SCHEDULER_EVERY_20_MINUTES,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	action_put(&test_action);
	measurement_start(60 * 60, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 1, 0, 20, 0)},
		{TM_INIT(110, 0, 1, 0, 40, 0)},
		{TM_INIT(110, 0, 1, 1, 0, 0)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_once_an_hour)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = BT_MESH_SCHEDULER_ANY_HOUR,
			.minute = BT_MESH_SCHEDULER_ONCE_AN_HOUR,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_min = 59;

	action_put(&test_action);
	measurement_start(180 * 60, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 1, 1, INT_MAX, 0)},
		{TM_INIT(110, 0, 1, 2, INT_MAX, 0)},
		{TM_INIT(110, 0, 1, 3, INT_MAX, 0)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_any_minute)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = BT_MESH_SCHEDULER_ANY_MINUTE,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	action_put(&test_action);
	measurement_start(3 * 60, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 1, 0, 1, 0)},
		{TM_INIT(110, 0, 1, 0, 2, 0)},
		{TM_INIT(110, 0, 1, 0, 3, 0)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_exact_time_hour_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = 2,
			.hour = 9,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_mday = 1;
	start_tm.tm_hour = 10;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 23ll, 1);

	struct tm expected = {
		TM_INIT(110, 0, 2, 9, 0, 0)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_once_a_day)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = BT_MESH_SCHEDULER_ONCE_A_DAY,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_hour = 23;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 72ll, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 2, INT_MAX, 0, 0)},
		{TM_INIT(110, 0, 3, INT_MAX, 0, 0)},
		{TM_INIT(110, 0, 4, INT_MAX, 0, 0)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_any_hour_day_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = BT_MESH_SCHEDULER_ANY_HOUR,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_hour = 23;

	action_put(&test_action);
	measurement_start(60 * 60 * 3, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 0, 2, 0, 0, 0)},
		{TM_INIT(110, 0, 2, 1, 0, 0)},
		{TM_INIT(110, 0, 2, 2, 0, 0)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_any_hour_month_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = BT_MESH_SCHEDULER_ANY_HOUR,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_mday = 31;
	start_tm.tm_hour = 23;

	action_put(&test_action);
	measurement_start(60 * 60 * 3, 3);

	struct tm expected[3] = {
		{TM_INIT(110, 1, 1, 0, 0, 0)},
		{TM_INIT(110, 1, 1, 1, 0, 0)},
		{TM_INIT(110, 1, 1, 2, 0, 0)}
	};

	expected_tm_check(fired_tm, expected, 3);
}

ZTEST(scheduler_timing, test_exact_time_day_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = 1,
			.hour = 0,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_mday = 30;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 24ll * 2ll, 1);

	struct tm expected = {
		TM_INIT(110, 1, 1, 0, 0, 0)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_any_day_week_gap)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = 0,
			.second = 0,
			.day_of_week = BT_MESH_SCHEDULER_SAT | BT_MESH_SCHEDULER_SUN,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	/* 7th of Jan 2010 - Thursday */
	start_tm.tm_mday = 7;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 24ll * (3ll + 7ll), 4);

	struct tm expected[4] = {
		{TM_INIT(110, 0, 9, 0, 0, 0)},
		{TM_INIT(110, 0, 10, 0, 0, 0)},
		{TM_INIT(110, 0, 16, 0, 0, 0)},
		{TM_INIT(110, 0, 17, 0, 0, 0)}
	};

	expected_tm_check(fired_tm, expected, 4);
}

ZTEST(scheduler_timing, test_any_day_week_gap_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = 0,
			.second = 0,
			.day_of_week = BT_MESH_SCHEDULER_MON,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	/* 9th of Jan 2010 - Saturday */
	start_tm.tm_mday = 9;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 24ll * (3ll + 7ll), 2);

	struct tm expected[2] = {
		{TM_INIT(110, 0, 11, 0, 0, 0)},
		{TM_INIT(110, 0, 18, 0, 0, 0)}
	};

	expected_tm_check(fired_tm, expected, 2);
}

ZTEST(scheduler_timing, test_any_day_week_gap_ovflw_next_month)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = ANY_MONTH,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = 0,
			.second = 0,
			.day_of_week = BT_MESH_SCHEDULER_MON,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	/* 30th of Jan 2010 - Saturday */
	start_tm.tm_mday = 30;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 24ll * 3ll, 1);

	struct tm expected = {
		TM_INIT(110, 1, 1, 0, 0, 0)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_any_day_month_gap)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = 10,
			.month  = BT_MESH_SCHEDULER_FEB,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_mday = 30;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 24ll * 2, 1);

	struct tm expected = {
		TM_INIT(110, 1, 1, 0, 0, 0)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_month_ovflw)
{
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = BT_MESH_SCHEDULER_JAN | BT_MESH_SCHEDULER_DEC,
			.day = BT_MESH_SCHEDULER_ANY_DAY,
			.hour = 0,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_year = 110;
	start_tm.tm_mon = 11;
	start_tm.tm_mday = 31;

	action_put(&test_action);
	measurement_start(60ll * 60ll * 24ll, 1);

	struct tm expected = {
		TM_INIT(111, 0, 1, 0, 0, 0)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST(scheduler_timing, test_exact_time_general_ovflw)
{ /* happy new year */
	const struct bt_mesh_schedule_entry test_action = {
			.year = BT_MESH_SCHEDULER_ANY_YEAR,
			.month  = ANY_MONTH,
			.day = 1,
			.hour = 0,
			.minute = 0,
			.second = 0,
			.day_of_week = ANY_DAY_OF_WEEK,
			.action = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 0,
			.scene_number = 1
	};

	start_tm.tm_year = 110;
	start_tm.tm_mon = 11;
	start_tm.tm_mday = 31;
	start_tm.tm_hour = 23;
	start_tm.tm_min = 59;
	start_tm.tm_sec = 59;

	action_put(&test_action);
	measurement_start(1, 1);

	struct tm expected = {
		TM_INIT(111, 0, 1, 0, 0, 0)
	};

	expected_tm_check(fired_tm, &expected, 1);
}

ZTEST_SUITE(scheduler_timing, NULL, NULL, tc_setup, tc_teardown, NULL);
