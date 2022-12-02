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

#define DELTA_TIME   1
#define SUBSEC_STEPS 256

/* item of struct tm */
#define ISTM(year, month, day, hour, minute, second) \
		.tm_year = (year),  \
		.tm_mon = (month),  \
		.tm_mday = (day),   \
		.tm_hour = (hour),  \
		.tm_min = (minute), \
		.tm_sec = (second)

#define ANY_MONTH (BT_MESH_SCHEDULER_JAN | BT_MESH_SCHEDULER_FEB | \
	BT_MESH_SCHEDULER_MAR | BT_MESH_SCHEDULER_APR | BT_MESH_SCHEDULER_MAY | \
	BT_MESH_SCHEDULER_JUN | BT_MESH_SCHEDULER_JUL | BT_MESH_SCHEDULER_AUG | \
	BT_MESH_SCHEDULER_SEP | BT_MESH_SCHEDULER_OCT | BT_MESH_SCHEDULER_NOV | \
	BT_MESH_SCHEDULER_DEC)

#define ANY_DAY_OF_WEEK (BT_MESH_SCHEDULER_MON | BT_MESH_SCHEDULER_TUE | \
	BT_MESH_SCHEDULER_WED | BT_MESH_SCHEDULER_THU | BT_MESH_SCHEDULER_FRI | \
	BT_MESH_SCHEDULER_SAT | BT_MESH_SCHEDULER_SUN)

static struct bt_mesh_scene_srv scene_srv;
static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);
static struct bt_mesh_scheduler_srv scheduler_srv =
	BT_MESH_SCHEDULER_SRV_INIT(NULL, &time_srv);

K_SEM_DEFINE(action_fired, 0, 1);

static struct bt_mesh_model mock_sched_model = {
	.user_data = &scheduler_srv,
	.elem_idx = 1,
};

static struct bt_mesh_model mock_scene_model = {
	.user_data = &scene_srv,
	.elem_idx = 1,
};

static struct bt_mesh_elem dummy_elem;
static struct tm start_tm;

static int gfire_cnt;
static struct tm *fired_tm;
static int galloc_cnt;

static inline uint64_t tai_to_ms(const struct bt_mesh_time_tai *tai)
{
	return MSEC_PER_SEC * tai->sec +
	       (MSEC_PER_SEC * tai->subsec) / SUBSEC_STEPS;
}

static inline struct bt_mesh_time_tai tai_at(int64_t uptime)
{
	int64_t steps = (SUBSEC_STEPS * uptime) / MSEC_PER_SEC;

	return (struct bt_mesh_time_tai) {
		.sec = (steps / SUBSEC_STEPS),
		.subsec = steps,
	};
}

/* redefined mocks */
uint8_t model_transition_encode(int32_t transition_time)
{
	return 0;
}

int32_t model_transition_decode(uint8_t encoded_transition)
{
	return 0;
}

struct bt_mesh_elem *bt_mesh_model_elem(struct bt_mesh_model *mod)
{
	return &dummy_elem;
}

struct bt_mesh_elem *bt_mesh_elem_find(uint16_t addr)
{
	return NULL;
}

struct bt_mesh_model *bt_mesh_model_find(const struct bt_mesh_elem *elem,
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

int bt_mesh_msg_send(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
	       struct net_buf_simple *buf)
{
	return 0;
}

int _bt_mesh_time_srv_update_handler(struct bt_mesh_model *model)
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

static void setup(void)
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

static void teardown(void)
{
	if (fired_tm) {
		free(fired_tm);
	}
	zassert_not_null(_bt_mesh_scheduler_srv_cb.reset,
			 "Reset cb is null");
	_bt_mesh_scheduler_srv_cb.reset(&mock_sched_model);
}

static void start_time_adjust(void)
{
	int64_t uptime = k_uptime_get();
	struct bt_mesh_time_tai tai;

	zassert_ok(ts_to_tai(&tai, &start_tm), "cannot convert tai time");

	int64_t tmp =  tai_to_ms(&tai);

	tmp -= uptime;
	tai = tai_at(tmp);
	tai.sec++; /* 1 sec is lost during recalculation */
	tai_to_ts(&tai, &start_tm);
}

static void action_put(const struct bt_mesh_schedule_entry *test_action)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);

	net_buf_simple_init(&buf, 0);
	scheduler_action_pack(&buf, 0, test_action);

	start_time_adjust();

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

static void expected_tm_check(struct tm *expected, int steps)
{
	for (int i = 0; i < steps; i++) {
		zassert_equal(fired_tm[i].tm_year, expected[i].tm_year,
			"real year %d is not equal to expected %d on step %d",
			fired_tm[i].tm_year, expected[i].tm_year, i + 1);

		zassert_equal(fired_tm[i].tm_mon, expected[i].tm_mon,
			"real month %d is not equal to expected %d on step %d",
			fired_tm[i].tm_mon, expected[i].tm_mon, i + 1);

		zassert_equal(fired_tm[i].tm_mday, expected[i].tm_mday,
			"real day %d is not equal to expected %d on step %d",
			fired_tm[i].tm_mday, expected[i].tm_mday, i + 1);

		if (expected[i].tm_hour == INT_MAX) {
			zassert_within(fired_tm[i].tm_hour, 0, 23,
				"real hour %d is not within expected 0-23 range on step %d",
				fired_tm[i].tm_hour, i + 1);
		} else {
			zassert_equal(fired_tm[i].tm_hour, expected[i].tm_hour,
				"real hour %d is not equal to expected %d on step %d",
				fired_tm[i].tm_hour, expected[i].tm_hour, i + 1);
		}

		if (expected[i].tm_min == INT_MAX) {
			zassert_within(fired_tm[i].tm_min, 0, 59,
				"real minute %d is not within expected 0-59 range on step %d",
				fired_tm[i].tm_min, i + 1);
		} else {
			zassert_equal(fired_tm[i].tm_min, expected[i].tm_min,
				"real minute %d is not equal to expected %d on step %d",
				fired_tm[i].tm_min, expected[i].tm_min, i + 1);
		}

		if (expected[i].tm_sec == INT_MAX) {
			zassert_within(fired_tm[i].tm_sec, 0, 59,
				"real second %d is not within expected 0-59 range on step %d",
				fired_tm[i].tm_sec, i + 1);
		} else {
			zassert_equal(fired_tm[i].tm_sec, expected[i].tm_sec,
				"real second %d is not equal to expected %d on step %d",
				fired_tm[i].tm_sec, expected[i].tm_sec, i + 1);
		}
	}
}

static void test_exact_time_simple(void)
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
		ISTM(110, 0, 1, 0, 0, 30)
	};

	expected_tm_check(&expected, 1);
}

static void test_exact_time_second_ovflw(void)
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
		ISTM(110, 0, 1, 10, 1, 5)
	};

	expected_tm_check(&expected, 1);
}

static void test_every_15_seconds(void)
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
		{ISTM(110, 0, 1, 0, 0, 15)},
		{ISTM(110, 0, 1, 0, 0, 30)},
		{ISTM(110, 0, 1, 0, 0, 45)},
		{ISTM(110, 0, 1, 0, 1, 0)}
	};

	expected_tm_check(expected, 4);
}

static void test_every_20_seconds(void)
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
		{ISTM(110, 0, 1, 0, 0, 20)},
		{ISTM(110, 0, 1, 0, 0, 40)},
		{ISTM(110, 0, 1, 0, 1, 0)}
	};

	expected_tm_check(expected, 3);
}

static void test_once_a_minute(void)
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
		{ISTM(110, 0, 1, 0, 1, INT_MAX)},
		{ISTM(110, 0, 1, 0, 2, INT_MAX)},
		{ISTM(110, 0, 1, 0, 3, INT_MAX)}
	};

	expected_tm_check(expected, 3);
}

static void test_any_second(void)
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
		{ISTM(110, 0, 1, 0, 0, 1)},
		{ISTM(110, 0, 1, 0, 0, 2)},
		{ISTM(110, 0, 1, 0, 0, 3)}
	};

	expected_tm_check(expected, 3);
}

static void test_exact_time_minute_ovflw(void)
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
		ISTM(110, 0, 1, 11, 5, 0)
	};

	expected_tm_check(&expected, 1);
}

static void test_every_15_minutes(void)
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
		{ISTM(110, 0, 1, 0, 15, 0)},
		{ISTM(110, 0, 1, 0, 30, 0)},
		{ISTM(110, 0, 1, 0, 45, 0)},
		{ISTM(110, 0, 1, 1, 0, 0)}
	};

	expected_tm_check(expected, 4);
}

static void test_every_20_minutes(void)
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
		{ISTM(110, 0, 1, 0, 20, 0)},
		{ISTM(110, 0, 1, 0, 40, 0)},
		{ISTM(110, 0, 1, 1, 0, 0)}
	};

	expected_tm_check(expected, 3);
}

static void test_once_an_hour(void)
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
		{ISTM(110, 0, 1, 1, INT_MAX, 0)},
		{ISTM(110, 0, 1, 2, INT_MAX, 0)},
		{ISTM(110, 0, 1, 3, INT_MAX, 0)}
	};

	expected_tm_check(expected, 3);
}

static void test_any_minute(void)
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
		{ISTM(110, 0, 1, 0, 1, 0)},
		{ISTM(110, 0, 1, 0, 2, 0)},
		{ISTM(110, 0, 1, 0, 3, 0)}
	};

	expected_tm_check(expected, 3);
}

static void test_exact_time_hour_ovflw(void)
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
		ISTM(110, 0, 2, 9, 0, 0)
	};

	expected_tm_check(&expected, 1);
}

static void test_once_a_day(void)
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
		{ISTM(110, 0, 2, INT_MAX, 0, 0)},
		{ISTM(110, 0, 3, INT_MAX, 0, 0)},
		{ISTM(110, 0, 4, INT_MAX, 0, 0)}
	};

	expected_tm_check(expected, 3);
}

static void test_any_hour_day_ovflw(void)
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
		{ISTM(110, 0, 2, 0, 0, 0)},
		{ISTM(110, 0, 2, 1, 0, 0)},
		{ISTM(110, 0, 2, 2, 0, 0)}
	};

	expected_tm_check(expected, 3);
}

static void test_any_hour_month_ovflw(void)
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
		{ISTM(110, 1, 1, 0, 0, 0)},
		{ISTM(110, 1, 1, 1, 0, 0)},
		{ISTM(110, 1, 1, 2, 0, 0)}
	};

	expected_tm_check(expected, 3);
}

static void test_exact_time_day_ovflw(void)
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
		ISTM(110, 1, 1, 0, 0, 0)
	};

	expected_tm_check(&expected, 1);
}

static void test_any_day_week_gap(void)
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
		{ISTM(110, 0, 9, 0, 0, 0)},
		{ISTM(110, 0, 10, 0, 0, 0)},
		{ISTM(110, 0, 16, 0, 0, 0)},
		{ISTM(110, 0, 17, 0, 0, 0)}
	};

	expected_tm_check(expected, 4);
}

static void test_any_day_week_gap_ovflw(void)
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
		{ISTM(110, 0, 11, 0, 0, 0)},
		{ISTM(110, 0, 18, 0, 0, 0)}
	};

	expected_tm_check(expected, 2);
}

static void test_any_day_week_gap_ovflw_next_month(void)
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
		ISTM(110, 1, 1, 0, 0, 0)
	};

	expected_tm_check(&expected, 1);
}

static void test_any_day_month_gap(void)
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
		ISTM(110, 1, 1, 0, 0, 0)
	};

	expected_tm_check(&expected, 1);
}

static void test_month_ovflw(void)
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
		ISTM(111, 0, 1, 0, 0, 0)
	};

	expected_tm_check(&expected, 1);
}

static void test_exact_time_general_ovflw(void)
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
		ISTM(111, 0, 1, 0, 0, 0)
	};

	expected_tm_check(&expected, 1);
}

void test_main(void)
{
	ztest_test_suite(scheduler_test,
		ztest_unit_test_setup_teardown(test_exact_time_simple, setup, teardown),
		ztest_unit_test_setup_teardown(test_exact_time_second_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_every_15_seconds, setup, teardown),
		ztest_unit_test_setup_teardown(test_every_20_seconds, setup, teardown),
		ztest_unit_test_setup_teardown(test_once_a_minute, setup, teardown),
		ztest_unit_test_setup_teardown(test_any_second, setup, teardown),
		ztest_unit_test_setup_teardown(test_exact_time_minute_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_every_15_minutes, setup, teardown),
		ztest_unit_test_setup_teardown(test_every_20_minutes, setup, teardown),
		ztest_unit_test_setup_teardown(test_once_an_hour, setup, teardown),
		ztest_unit_test_setup_teardown(test_any_minute, setup, teardown),
		ztest_unit_test_setup_teardown(test_exact_time_hour_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_once_a_day, setup, teardown),
		ztest_unit_test_setup_teardown(test_any_hour_day_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_any_hour_month_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_exact_time_day_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_any_day_week_gap, setup, teardown),
		ztest_unit_test_setup_teardown(test_any_day_week_gap_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_any_day_week_gap_ovflw_next_month,
				setup, teardown),
		ztest_unit_test_setup_teardown(test_any_day_month_gap, setup, teardown),
		ztest_unit_test_setup_teardown(test_month_ovflw, setup, teardown),
		ztest_unit_test_setup_teardown(test_exact_time_general_ovflw, setup, teardown)
		);

	ztest_run_test_suite(scheduler_test);
}
