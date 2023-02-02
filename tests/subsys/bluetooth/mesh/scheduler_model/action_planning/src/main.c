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

K_SEM_DEFINE(action_fired, 0, 1);

static struct tm start_tm;
static int gfire_cnt;
static struct tm *fired_tm;
static int galloc_cnt;

struct test_onoff_srv {
	struct bt_mesh_onoff_srv srv;
	int elem_idx;
};

struct test_scene_srv {
	struct bt_mesh_scene_srv srv;
	int elem_idx;
};

struct sched_evt {
	uint8_t evt;
	int32_t transition_time;
	uint16_t scene_number;
	int elem_idx;
};

static struct {
	int evt_cnt;
	int curr_idx;
	struct sched_evt *events;
} sched_evt_ctx;

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

static void sched_evt_ctx_init(struct sched_evt *evt_list, int evt_cnt)
{
	sched_evt_ctx.curr_idx = 0;
	sched_evt_ctx.events = evt_list;
	sched_evt_ctx.evt_cnt = evt_cnt;
}

static struct sched_evt *sched_evt_next(void)
{
	zassert_false(sched_evt_ctx.curr_idx >= sched_evt_ctx.evt_cnt, "No more expected events");

	return &sched_evt_ctx.events[sched_evt_ctx.curr_idx++];
}

static bool is_sched_evt_empty(void)
{
	return sched_evt_ctx.curr_idx >= sched_evt_ctx.evt_cnt;
}

static struct bt_mesh_time_srv time_srv = BT_MESH_TIME_SRV_INIT(NULL);

static void onoff_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      const struct bt_mesh_onoff_set *set, struct bt_mesh_onoff_status *rsp)
{
	struct sched_evt *evt = sched_evt_next();
	struct test_onoff_srv *ptr = CONTAINER_OF(srv, struct test_onoff_srv, srv);

	zassert_equal(evt->elem_idx, ptr->elem_idx, "Wrong sequence of onoff events");
	zassert_equal(set->on_off ? BT_MESH_SCHEDULER_TURN_ON : BT_MESH_SCHEDULER_TURN_OFF,
		      evt->evt, "Wrong onoff event triggered");
	zassert_equal(evt->transition_time, set->transition->time,
		      "Unexpected transition time");

	if (fired_tm) {
		fired_tm[galloc_cnt] = *bt_mesh_time_srv_localtime(&time_srv, k_uptime_get());
		galloc_cnt++;
	}

	if (is_sched_evt_empty()) {
		k_sem_give(&action_fired);
	}
}

static void onoff_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		      struct bt_mesh_onoff_status *rsp)
{
}

struct bt_mesh_onoff_srv_handlers onoff_srv_handlers = {
	.set = onoff_set,
	.get = onoff_get,
};

/** Mocking mesh composition data
 *
 * The mocked composition data consists of 5 elements with
 * 2 Scheduler servers, 5 Generic onoff servers and 5 Scene
 * servers, distributed as following:
 *
 * |      1     |     2     |     3     |      4     |     5     |
 * |:----------:|:---------:|:---------:|:----------:|:---------:|
 * | Sched. Srv | Onoff Srv | Onoff Srv | Sched. Srv | Onoff Srv |
 * |  Onoff Srv | Scene Srv | Scene Srv |  Onoff Srv | Scene Srv |
 * |  Scene Srv |           |           |  Scene Srv |           |
 */

static struct bt_mesh_scheduler_srv sched_srv[2] = {
	BT_MESH_SCHEDULER_SRV_INIT(NULL, &time_srv),
	BT_MESH_SCHEDULER_SRV_INIT(NULL, &time_srv),
};

struct test_onoff_srv onoff_srv[5] = {
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_srv_handlers), .elem_idx = 0 },
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_srv_handlers), .elem_idx = 1 },
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_srv_handlers), .elem_idx = 2 },
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_srv_handlers), .elem_idx = 3 },
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_srv_handlers), .elem_idx = 4 }
};

struct test_scene_srv scene_srv[5] = {
	{ .elem_idx = 0 }, { .elem_idx = 1 }, { .elem_idx = 2 },
	{ .elem_idx = 3 }, { .elem_idx = 4 },
};

static struct bt_mesh_model mock_models_elem1[3] = {
	{
		.user_data = &sched_srv[0],
		.id = BT_MESH_MODEL_ID_SCHEDULER_SRV,
	},
	{
		.user_data = &onoff_srv[0].srv,
		.id = BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
	},
	{
		.user_data = &scene_srv[0].srv,
		.id = BT_MESH_MODEL_ID_SCENE_SRV,
	},
};

static struct bt_mesh_model mock_models_elem2[2] = {
	{
		.user_data = &onoff_srv[1].srv,
		.id = BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
	},
	{
		.user_data = &scene_srv[1].srv,
		.id = BT_MESH_MODEL_ID_SCENE_SRV,
	},
};

static struct bt_mesh_model mock_models_elem3[2] = {
	{
		.user_data = &onoff_srv[2].srv,
		.id = BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
	},
	{
		.user_data = &scene_srv[2].srv,
		.id = BT_MESH_MODEL_ID_SCENE_SRV,
	},
};

static struct bt_mesh_model mock_models_elem4[3] = {
	{
		.user_data = &sched_srv[1],
		.id = BT_MESH_MODEL_ID_SCHEDULER_SRV,
	},
	{
		.user_data = &onoff_srv[3].srv,
		.id = BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
	},
	{
		.user_data = &scene_srv[3].srv,
		.id = BT_MESH_MODEL_ID_SCENE_SRV,
	},
};

static struct bt_mesh_model mock_models_elem5[2] = {
	{
		.user_data = &onoff_srv[4].srv,
		.id = BT_MESH_MODEL_ID_GEN_ONOFF_SRV,
	},
	{
		.user_data = &scene_srv[4].srv,
		.id = BT_MESH_MODEL_ID_SCENE_SRV,
	},
};

static struct bt_mesh_elem mock_elems[5] = {
	{ .addr = 1, .model_count = 3, .models = mock_models_elem1 },
	{ .addr = 2, .model_count = 2, .models = mock_models_elem2 },
	{ .addr = 3, .model_count = 2, .models = mock_models_elem3 },
	{ .addr = 4, .model_count = 3, .models = mock_models_elem4 },
	{ .addr = 5, .model_count = 2, .models = mock_models_elem5 }
};

struct mock_comp_data {
	size_t elem_count;
	struct bt_mesh_elem *elem;
} mock_comp = { .elem_count = 5, .elem = mock_elems };

static struct bt_mesh_model *sched_mod_elem1 = &mock_models_elem1[0];
static struct bt_mesh_model *sched_mod_elem4 = &mock_models_elem4[0];

static void mod_elem_idx_prep(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(mock_elems); i++) {
		for (size_t j = 0; j < mock_elems[i].model_count; j++) {
			mock_elems[i].models[j].elem_idx = i;
		}
	}
}

/* Redefined mocks */

struct bt_mesh_elem *bt_mesh_model_elem(struct bt_mesh_model *mod)
{
	return &mock_comp.elem[mod->elem_idx];
}

struct bt_mesh_elem *bt_mesh_elem_find(uint16_t addr)
{
	uint16_t index;

	if (!BT_MESH_ADDR_IS_UNICAST(addr)) {
		return NULL;
	}

	index = addr - mock_comp.elem[0].addr;
	if (index >= mock_comp.elem_count) {
		return NULL;
	}
	return &mock_comp.elem[index];
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
	struct sched_evt *evt = sched_evt_next();
	struct test_scene_srv *ptr = CONTAINER_OF(srv, struct test_scene_srv, srv);

	zassert_equal(evt->elem_idx, ptr->elem_idx, "Wrong sequence of scene events");
	zassert_equal(BT_MESH_SCHEDULER_SCENE_RECALL, evt->evt, "Incorrect scene event");
	zassert_equal(evt->transition_time, transition->time, "Unexpected transition time");
	zassert_equal(evt->scene_number, scene, "Incorrect Scene number");

	if (fired_tm) {
		fired_tm[galloc_cnt] = *bt_mesh_time_srv_localtime(&time_srv, k_uptime_get());
		galloc_cnt++;
	}

	if (is_sched_evt_empty()) {
		k_sem_give(&action_fired);
	}
	return 0;
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

void bt_mesh_model_msg_init(struct net_buf_simple *msg, uint32_t opcode)
{
	net_buf_simple_init(msg, 0);
}

int64_t bt_mesh_time_srv_mktime(struct bt_mesh_time_srv *srv, struct tm *timeptr)
{
	return (timeutil_timegm64(timeptr) - timeutil_timegm64(&start_tm)) * 1000;
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
/* Redefined mocks */

static void setup(void)
{
	mod_elem_idx_prep();

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

	_bt_mesh_scheduler_srv_cb.init(sched_mod_elem1);
	_bt_mesh_scheduler_srv_cb.init(sched_mod_elem4);
}

static void teardown(void)
{
	if (fired_tm) {
		free(fired_tm);
	}
	zassert_not_null(_bt_mesh_scheduler_srv_cb.reset,
			 "Reset cb is null");

	_bt_mesh_scheduler_srv_cb.reset(sched_mod_elem1);
	_bt_mesh_scheduler_srv_cb.reset(sched_mod_elem4);
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

static void action_put(const struct bt_mesh_schedule_entry *test_action,
		       struct bt_mesh_model *sched_mod)
{
	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
				 BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET);

	net_buf_simple_init(&buf, 0);
	scheduler_action_pack(&buf, 0, test_action);

	start_time_adjust();

	zassert_false(_bt_mesh_scheduler_setup_srv_op[1].func(sched_mod, NULL, &buf),
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

static void test_first_sched_turn_on(void)
{
	/** Send a TURN_ON action to the sched srv in
	 * element 1 of the mocked composition data.
	 *
	 * Expect the onoff srv in elem 1, 2 and 3 to
	 * trigger in sequence, omitting the onoff servers
	 * in elem 4 and 5 due to the precence of a sched
	 * srv in element 4. Also verifies the correctness
	 * of the injected parameters and the number and
	 * timing of events.
	 */

	const struct bt_mesh_schedule_entry test_action = {
		.year = 10,
		.month = ANY_MONTH,
		.day = 1,
		.hour = 0,
		.minute = 0,
		.second = 30,
		.day_of_week = ANY_DAY_OF_WEEK,
		.action = BT_MESH_SCHEDULER_TURN_ON,
		.transition_time = model_transition_encode(2500),
		.scene_number = 1,
	};

	struct sched_evt evt_list[] = {
		{ .elem_idx = 0, .evt = BT_MESH_SCHEDULER_TURN_ON, .transition_time = 2500 },
		{ .elem_idx = 1, .evt = BT_MESH_SCHEDULER_TURN_ON, .transition_time = 2500 },
		{ .elem_idx = 2, .evt = BT_MESH_SCHEDULER_TURN_ON, .transition_time = 2500 },
	};

	sched_evt_ctx_init(evt_list, ARRAY_SIZE(evt_list));

	action_put(&test_action, sched_mod_elem1);
	measurement_start(60, ARRAY_SIZE(evt_list));

	struct tm expected[3] = {
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
	};

	expected_tm_check(expected, 1);
}

static void test_second_sched_turn_off(void)
{
	/** Send a TURN_OFF action to the sched srv in
	 * element 4 of the mocked composition data.
	 *
	 * Expect the onoff srv in elem 4 and 5 to trigger
	 * in sequence. Also verifies the correctness
	 * of the injected parameters and the number and
	 * timing of events.
	 */

	const struct bt_mesh_schedule_entry test_action = {
		.year = 10,
		.month = ANY_MONTH,
		.day = 1,
		.hour = 0,
		.minute = 0,
		.second = 30,
		.day_of_week = ANY_DAY_OF_WEEK,
		.action = BT_MESH_SCHEDULER_TURN_OFF,
		.transition_time = model_transition_encode(100),
		.scene_number = 1,
	};

	struct sched_evt evt_list[] = {
		{ .elem_idx = 3, .evt = BT_MESH_SCHEDULER_TURN_OFF, .transition_time = 100 },
		{ .elem_idx = 4, .evt = BT_MESH_SCHEDULER_TURN_OFF, .transition_time = 100 },
	};

	sched_evt_ctx_init(evt_list, ARRAY_SIZE(evt_list));

	action_put(&test_action, sched_mod_elem4);
	measurement_start(60, ARRAY_SIZE(evt_list));

	struct tm expected[2] = {
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
	};

	expected_tm_check(expected, 1);
}

static void test_first_sched_scene_recall(void)
{
	/** Send a SCENE_RECALL action to the sched srv in
	 * element 1 of the mocked composition data.
	 *
	 * Expect the scene srv in elem 1, 2 and 3 to
	 * trigger in sequence, omitting the scene servers
	 * in elem 4 and 5 due to the precence of a sched
	 * srv in element 4. Also verifies the correctness
	 * of the injected parameters and the number and
	 * timing of events.
	 */

	const struct bt_mesh_schedule_entry test_action = {
		.year = 10,
		.month = ANY_MONTH,
		.day = 1,
		.hour = 0,
		.minute = 0,
		.second = 30,
		.day_of_week = ANY_DAY_OF_WEEK,
		.action = BT_MESH_SCHEDULER_SCENE_RECALL,
		.transition_time = model_transition_encode(3000),
		.scene_number = 2,
	};

	struct sched_evt evt_list[] = {
		{
			.elem_idx = 0,
			.evt = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 3000,
			.scene_number = 2,
		},
		{
			.elem_idx = 1,
			.evt = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 3000,
			.scene_number = 2,
		},
		{
			.elem_idx = 2,
			.evt = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 3000,
			.scene_number = 2,
		},
	};

	sched_evt_ctx_init(evt_list, ARRAY_SIZE(evt_list));

	action_put(&test_action, sched_mod_elem1);
	measurement_start(60, ARRAY_SIZE(evt_list));

	struct tm expected[3] = {
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
	};

	expected_tm_check(fired_tm, expected, 3);
}

static void test_second_sched_scene_recall(void)
{
	/** Send a SCENE_RECALL action to the sched srv in
	 * element 4 of the mocked composition data.
	 *
	 * Expect the scene srv in elem 4 and 5 to trigger
	 * in sequence. Also verifies the correctness
	 * of the injected parameters and the number and
	 * timing of events.
	 */

	const struct bt_mesh_schedule_entry test_action = {
		.year = 10,
		.month = ANY_MONTH,
		.day = 1,
		.hour = 0,
		.minute = 0,
		.second = 30,
		.day_of_week = ANY_DAY_OF_WEEK,
		.action = BT_MESH_SCHEDULER_SCENE_RECALL,
		.transition_time = model_transition_encode(500),
		.scene_number = 4,
	};

	struct sched_evt evt_list[] = {
		{
			.elem_idx = 3,
			.evt = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 500,
			.scene_number = 4,
		},
		{
			.elem_idx = 4,
			.evt = BT_MESH_SCHEDULER_SCENE_RECALL,
			.transition_time = 500,
			.scene_number = 4,
		},
	};

	sched_evt_ctx_init(evt_list, ARRAY_SIZE(evt_list));

	action_put(&test_action, sched_mod_elem4);
	measurement_start(60, ARRAY_SIZE(evt_list));

	struct tm expected[2] = {
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
	};

	expected_tm_check(expected, 2);
}

static void test_second_sched_turn_off_recurring(void)
{
	/** Send a recurring TURN_OFF action to the sched
	 * srv in element 4 of the mocked composition data.
	 *
	 * Expect the onoff srv in elem 4 and 5 to trigger
	 * in sequence for every triggered sched event.
	 * Also verifies the correctness of the injected
	 * parameters and the number and timing of events.
	 */

	const struct bt_mesh_schedule_entry test_action = {
		.year = BT_MESH_SCHEDULER_ANY_YEAR,
		.month = ANY_MONTH,
		.day = BT_MESH_SCHEDULER_ANY_DAY,
		.hour = 0,
		.minute = BT_MESH_SCHEDULER_ANY_MINUTE,
		.second = BT_MESH_SCHEDULER_EVERY_15_SECONDS,
		.day_of_week = ANY_DAY_OF_WEEK,
		.action = BT_MESH_SCHEDULER_TURN_OFF,
		.transition_time = model_transition_encode(0),
		.scene_number = 1
	};

	struct sched_evt evt_list[] = {
		{ .elem_idx = 3, .evt = BT_MESH_SCHEDULER_TURN_OFF },
		{ .elem_idx = 4, .evt = BT_MESH_SCHEDULER_TURN_OFF },
		{ .elem_idx = 3, .evt = BT_MESH_SCHEDULER_TURN_OFF },
		{ .elem_idx = 4, .evt = BT_MESH_SCHEDULER_TURN_OFF },
		{ .elem_idx = 3, .evt = BT_MESH_SCHEDULER_TURN_OFF },
		{ .elem_idx = 4, .evt = BT_MESH_SCHEDULER_TURN_OFF },
		{ .elem_idx = 3, .evt = BT_MESH_SCHEDULER_TURN_OFF },
		{ .elem_idx = 4, .evt = BT_MESH_SCHEDULER_TURN_OFF },
	};

	sched_evt_ctx_init(evt_list, ARRAY_SIZE(evt_list));
	action_put(&test_action, sched_mod_elem4);

	measurement_start(60, ARRAY_SIZE(evt_list));

	struct tm expected[8] = {
		{ ISTM(110, 0, 1, 0, 0, 15) },
		{ ISTM(110, 0, 1, 0, 0, 15) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 30) },
		{ ISTM(110, 0, 1, 0, 0, 45) },
		{ ISTM(110, 0, 1, 0, 0, 45) },
		{ ISTM(110, 0, 1, 0, 1, 0) },
		{ ISTM(110, 0, 1, 0, 1, 0) },
	};

	expected_tm_check(expected, 8);
}

void test_main(void)
{
	ztest_test_suite(scheduler_test_action_planning,
		ztest_unit_test_setup_teardown(test_first_sched_turn_on, setup, teardown),
		ztest_unit_test_setup_teardown(test_second_sched_turn_off, setup, teardown),
		ztest_unit_test_setup_teardown(test_first_sched_scene_recall, setup, teardown),
		ztest_unit_test_setup_teardown(test_second_sched_scene_recall, setup, teardown),
		ztest_unit_test_setup_teardown(test_second_sched_turn_off_recurring, setup,
					       teardown));

	ztest_run_test_suite(scheduler_test_action_planning);
}
