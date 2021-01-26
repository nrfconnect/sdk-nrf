/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/scheduler_srv.h>
#include <bluetooth/mesh/scene_srv.h>
#include <bluetooth/mesh/gen_onoff_srv.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <sys/math_extras.h>
#include <random/rand32.h>
#include "model_utils.h"
#include "time_util.h"
#include "scheduler_internal.h"
#include "mesh/net.h"
#include "mesh/access.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG_MODEL)
#define LOG_MODULE_NAME bt_mesh_scheduler_srv
#include "common/log.h"

#define MAX_DAY        0x1F
#define JANUARY           0
#define DECEMBER         11

static bool set_year(struct tm *sched_time,
		     struct tm *current_local,
		     struct bt_mesh_schedule_entry *entry);
static bool set_month(struct tm *sched_time,
		      struct tm *current_local,
		      struct bt_mesh_schedule_entry *entry);
static bool set_day(struct tm *sched_time,
		    struct tm *current_local,
		    struct bt_mesh_schedule_entry *entry,
		    struct bt_mesh_time_srv *srv);
static bool set_hour(struct tm *sched_time,
		     struct tm *current_local,
		     struct bt_mesh_schedule_entry *entry,
		     struct bt_mesh_time_srv *srv);
static bool set_minute(struct tm *sched_time,
		       struct tm *current_local,
		       struct bt_mesh_schedule_entry *entry,
		       struct bt_mesh_time_srv *srv);
static bool set_second(struct tm *sched_time,
		       struct tm *current_local,
		       struct bt_mesh_schedule_entry *entry,
		       struct bt_mesh_time_srv *srv);

static bool revise_year(struct tm *sched_time,
			struct tm *current_local,
			struct bt_mesh_schedule_entry *entry,
			int number_years)
{
	struct tm revised_time = *current_local;

	revised_time.tm_year += number_years;
	return set_year(sched_time, &revised_time, entry);
}

static bool revise_month(struct tm *sched_time,
			 struct tm *current_local,
			 struct bt_mesh_schedule_entry *entry,
			 int number_days)
{
	int days[12] = {31,
		is_leap_year(current_local->tm_year + TM_START_YEAR) ? 29 : 28,
		31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	struct tm revised_time = *current_local;
	bool is_year_revised = false;

	revised_time.tm_mday += number_days;

	if (revised_time.tm_mday > days[revised_time.tm_mon]) {
		revised_time.tm_mday -= days[revised_time.tm_mon];
		if (revised_time.tm_mon == DECEMBER) {
			revised_time.tm_mon = JANUARY;
			revised_time.tm_year++;
			is_year_revised = true;
		} else {
			revised_time.tm_mon++;
		}
	}

	if (is_year_revised) {
		if (!revise_year(sched_time, current_local, entry, 1)) {
			return false;
		}
	}

	sched_time->tm_mday = revised_time.tm_mday;
	if (revised_time.tm_mon != current_local->tm_mon) {
		return set_month(sched_time, &revised_time, entry);
	}

	return true;
}

static bool revise_day(struct tm *sched_time,
		       struct tm *current_local,
		       struct bt_mesh_schedule_entry *entry,
		       struct bt_mesh_time_srv *srv,
		       int number_days)
{
	struct tm revised_time = *current_local;

	revised_time.tm_mday += number_days;
	return set_day(sched_time, &revised_time, entry, srv);
}

static bool revise_hour(struct tm *sched_time,
			struct tm *current_local,
			struct bt_mesh_schedule_entry *entry,
			struct bt_mesh_time_srv *srv,
			int number_hours)
{
	struct tm revised_time = *current_local;

	revised_time.tm_hour += number_hours;
	return set_hour(sched_time, &revised_time, entry, srv);
}

static bool revise_minute(struct tm *sched_time,
			  struct tm *current_local,
			  struct bt_mesh_schedule_entry *entry,
			  struct bt_mesh_time_srv *srv,
			  int number_minutes)
{
	struct tm revised_time = *current_local;

	revised_time.tm_min += number_minutes;
	return set_minute(sched_time, &revised_time, entry, srv);
}

static bool set_year(struct tm *sched_time,
		     struct tm *current_local,
		     struct bt_mesh_schedule_entry *entry)
{
	uint8_t current_year = current_local->tm_year % 100;
	uint8_t diff = entry->year >= current_year ?
		entry->year - current_year : 100 - current_year + entry->year;

	sched_time->tm_year = entry->year == BT_MESH_SCHEDULER_ANY_YEAR ?
			current_local->tm_year : current_local->tm_year + diff;
	return true;
}

static bool set_month(struct tm *sched_time,
		      struct tm *current_local,
		      struct bt_mesh_schedule_entry *entry)
{
	int month = entry->month;

	if (!month) {
		return false;
	}

	if (sched_time->tm_year == current_local->tm_year) {
		month &= (0xfff << current_local->tm_mon);
		if (!month) {
			if (!revise_year(sched_time, current_local, entry, 1)) {
				return false;
			}
			month = entry->month;
		}
	}

	sched_time->tm_mon = u32_count_trailing_zeros(month);
	return true;
}

static int get_day_of_week(int year, int month, int day)
{
	int day_cnt = 0;

	year += TM_START_YEAR;

	for (int i = TM_START_YEAR; i < year; i++) {
		day_cnt += is_leap_year(i) ? DAYS_LEAP_YEAR : DAYS_YEAR;
	}

	int days[12] = {31,
		is_leap_year(year) ? 29 : 28,
		31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	for (int i = 0; i < month; i++) {
		day_cnt += days[i];
	}

	day_cnt += day;
	return (day_cnt - 1) % WEEKDAY_CNT;
}

static bool set_day(struct tm *sched_time,
		    struct tm *current_local,
		    struct bt_mesh_schedule_entry *entry,
		    struct bt_mesh_time_srv *srv)
{
	if (entry->day < current_local->tm_mday &&
			entry->day != BT_MESH_SCHEDULER_ANY_DAY) {
		return false;
	}

	if (!entry->day_of_week) {
		return false;
	}

	sched_time->tm_mday = entry->day == BT_MESH_SCHEDULER_ANY_DAY ?
			current_local->tm_mday : entry->day;

	sched_time->tm_wday = get_day_of_week(sched_time->tm_year,
			sched_time->tm_mon, sched_time->tm_mday);

	if (entry->day_of_week & (1 << sched_time->tm_wday)) {
		return true;
	}

	if (entry->day == BT_MESH_SCHEDULER_ANY_DAY) {
		int rest_wday = entry->day_of_week >> sched_time->tm_wday;
		int delta = rest_wday ? u32_count_trailing_zeros(rest_wday) :
			u32_count_trailing_zeros(entry->day_of_week) +
			6 - sched_time->tm_wday;
		return revise_month(sched_time, current_local, entry, delta);
	}

	return false;
}

static bool set_hour(struct tm *sched_time,
		     struct tm *current_local,
		     struct bt_mesh_schedule_entry *entry,
		     struct bt_mesh_time_srv *srv)
{
	if (entry->hour < current_local->tm_hour) {
		return false;
	}

	if (entry->hour == BT_MESH_SCHEDULER_ONCE_A_DAY) {
		sched_time->tm_hour = sys_rand32_get() % 24;
		return revise_day(sched_time, current_local, entry, srv, 1);
	}

	sched_time->tm_hour = entry->hour == BT_MESH_SCHEDULER_ANY_HOUR ?
			current_local->tm_hour : entry->hour;

	return true;
}

static bool set_minute(struct tm *sched_time,
		       struct tm *current_local,
		       struct bt_mesh_schedule_entry *entry,
		       struct bt_mesh_time_srv *srv)
{
	if (entry->minute < current_local->tm_min) {
		return false;
	}

	bool ovflw = false;

	if (entry->minute == BT_MESH_SCHEDULER_EVERY_15_MINUTES) {
		sched_time->tm_min =
			15 * ceiling_fraction(current_local->tm_min + 1, 15);
		ovflw = current_local->tm_min == 60 ? true : false;
	} else if (entry->minute == BT_MESH_SCHEDULER_EVERY_20_MINUTES) {
		sched_time->tm_min =
			20 * ceiling_fraction(current_local->tm_min + 1, 20);
		ovflw = current_local->tm_min == 60 ? true : false;
	} else if (entry->minute == BT_MESH_SCHEDULER_ONCE_AN_HOUR) {
		sched_time->tm_min = sys_rand32_get() % 60;
		ovflw = true;
	} else {
		sched_time->tm_min =
			entry->minute == BT_MESH_SCHEDULER_ANY_MINUTE ?
				current_local->tm_min : entry->minute;
	}

	if (ovflw) {
		return revise_hour(sched_time, current_local, entry, srv, 1);
	}

	return true;
}

static bool set_second(struct tm *sched_time,
		       struct tm *current_local,
		       struct bt_mesh_schedule_entry *entry,
		       struct bt_mesh_time_srv *srv)
{
	if (entry->second < current_local->tm_sec) {
		return false;
	}

	bool ovflw = false;

	if (entry->second == BT_MESH_SCHEDULER_EVERY_15_SECONDS) {
		sched_time->tm_sec =
			15 * ceiling_fraction(current_local->tm_sec + 1, 15);
		ovflw = current_local->tm_sec == 60 ? true : false;
	} else if (entry->second == BT_MESH_SCHEDULER_EVERY_20_SECONDS) {
		sched_time->tm_sec =
			20 * ceiling_fraction(current_local->tm_sec + 1, 20);
		ovflw = current_local->tm_sec == 60 ? true : false;
	} else if (entry->second == BT_MESH_SCHEDULER_ONCE_A_MINUTE) {
		sched_time->tm_sec = sys_rand32_get() % 60;
		ovflw = true;
	} else {
		sched_time->tm_sec =
			entry->second == BT_MESH_SCHEDULER_ANY_SECOND ?
				current_local->tm_sec : entry->second;
	}

	if (ovflw) {
		return revise_minute(sched_time, current_local, entry, srv, 1);
	}

	return true;
}

static uint8_t get_least_time_index(struct bt_mesh_scheduler_srv *srv)
{
	uint8_t cnt = u32_count_trailing_zeros(srv->status_bitmap);
	uint8_t idx = cnt;

	while (++cnt < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		if (srv->status_bitmap & BIT(cnt)) {
			idx = srv->sched_tai[idx].sec >
				srv->sched_tai[cnt].sec ? cnt : idx;
		}
	}

	return MIN(BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT, idx);
}

static void run_scheduler(struct bt_mesh_scheduler_srv *srv)
{
	struct tm sched_time;
	int64_t current_uptime = k_uptime_get();
	uint8_t planned_idx = get_least_time_index(srv);

	if (planned_idx == BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return;
	}

	if (srv->idx != BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		k_delayed_work_cancel(&srv->delayed_work);
	}

	srv->idx = planned_idx;
	tai_to_ts(&srv->sched_tai[planned_idx], &sched_time);
	int64_t scheduled_uptime = bt_mesh_time_srv_mktime(srv->time_srv,
			&sched_time);
	k_delayed_work_submit(&srv->delayed_work,
		K_MSEC(scheduled_uptime > current_uptime ?
				scheduled_uptime - current_uptime : 0));
	BT_DBG("Scheduler started. Target uptime: %lld", scheduled_uptime);
}

static void schedule_action(struct bt_mesh_scheduler_srv *srv,
			    uint8_t idx)
{
	struct tm sched_time;
	struct bt_mesh_schedule_entry *entry = &srv->sch_reg[idx];

	int64_t current_uptime = k_uptime_get();
	struct tm *current_local = bt_mesh_time_srv_localtime(srv->time_srv,
			current_uptime);

	BT_DBG("Current uptime %lld", current_uptime);

	BT_DBG("Current time:");
	BT_DBG("        year: %d", current_local->tm_year);
	BT_DBG("       month: %d", current_local->tm_mon);
	BT_DBG("         day: %d", current_local->tm_mday);
	BT_DBG("        hour: %d", current_local->tm_hour);
	BT_DBG("      minute: %d", current_local->tm_min);
	BT_DBG("      second: %d", current_local->tm_sec);

	if (!set_year(&sched_time, current_local, entry)) {
		BT_DBG("Not accepted year %d", entry->year);
		return;
	}

	if (!set_month(&sched_time, current_local, entry)) {
		BT_DBG("Not accepted month %#2x", entry->month);
		return;
	}

	if (!set_day(&sched_time, current_local, entry, srv->time_srv)) {
		BT_DBG("Not accepted day %d or wday %#2x",
				entry->day, entry->day_of_week);
		return;
	}

	if (!set_hour(&sched_time, current_local, entry, srv->time_srv)) {
		BT_DBG("Not accepted hour %d", entry->hour);
		return;
	}

	if (!set_minute(&sched_time, current_local, entry, srv->time_srv)) {
		BT_DBG("Not accepted minute %d", entry->minute);
		return;
	}

	if (!set_second(&sched_time, current_local, entry, srv->time_srv)) {
		BT_DBG("Not accepted second %d", entry->second);
		return;
	}

	if (ts_to_tai(&srv->sched_tai[idx], &sched_time)) {
		BT_DBG("tm cannot be converted into TAI");
		return;
	}

	BT_DBG("Scheduled time:");
	BT_DBG("          year: %d", sched_time.tm_year);
	BT_DBG("         month: %d", sched_time.tm_mon);
	BT_DBG("           day: %d", sched_time.tm_mday);
	BT_DBG("          hour: %d", sched_time.tm_hour);
	BT_DBG("        minute: %d", sched_time.tm_min);
	BT_DBG("        second: %d", sched_time.tm_sec);

	WRITE_BIT(srv->status_bitmap, idx, 1);
}

static void scheduled_action_handle(struct k_work *work)
{
	struct bt_mesh_scheduler_srv *srv = CONTAINER_OF(work,
				struct bt_mesh_scheduler_srv,
				delayed_work);

	WRITE_BIT(srv->status_bitmap, srv->idx, 0);

	struct bt_mesh_model *next_sched_mod = NULL;
	uint16_t model_id = srv->sch_reg[srv->idx].action ==
				BT_MESH_SCHEDULER_SCENE_RECALL ?
		BT_MESH_MODEL_ID_SCENE_SRV : BT_MESH_MODEL_ID_GEN_ONOFF_SRV;
	struct bt_mesh_model_transition transition = {
		.time = model_transition_decode(
				srv->sch_reg[srv->idx].transition_time),
		.delay = 0
	};
	uint16_t scene = srv->sch_reg[srv->idx].scene_number;
	struct bt_mesh_elem *elem = bt_mesh_model_elem(srv->mod);

	BT_DBG("Scheduler action fired: %d", srv->sch_reg[srv->idx].action);

	do {
		struct bt_mesh_model *handled_mod =
				bt_mesh_model_find(elem, model_id);

		if (model_id == BT_MESH_MODEL_ID_SCENE_SRV &&
				handled_mod != NULL) {
			struct bt_mesh_scene_srv *scene_srv =
			(struct bt_mesh_scene_srv *)handled_mod->user_data;

			bt_mesh_scene_srv_set(scene_srv, scene, &transition);
			bt_mesh_scene_srv_pub(scene_srv, NULL);
			BT_DBG("Scene srv addr: %d recalled scene: %d",
				elem->addr,
				srv->sch_reg[srv->idx].scene_number);
		}

		if (model_id == BT_MESH_MODEL_ID_GEN_ONOFF_SRV  &&
				handled_mod != NULL) {
			struct bt_mesh_onoff_srv *onoff_srv =
			(struct bt_mesh_onoff_srv *)handled_mod->user_data;
			struct bt_mesh_onoff_set set = {
				.on_off = srv->sch_reg[srv->idx].action,
				.transition = &transition
			};
			struct bt_mesh_onoff_status status = { 0 };

			onoff_srv->handlers->set(onoff_srv,
					NULL, &set, &status);
			bt_mesh_onoff_srv_pub(onoff_srv, NULL, &status);
			BT_DBG("Onoff srv addr: %d set: %d",
				elem->addr,
				srv->sch_reg[srv->idx].action);
		}

		elem = BT_MESH_ADDR_IS_UNICAST(elem->addr + 1) ?
				bt_mesh_elem_find(elem->addr + 1) : NULL;

		if (elem) {
			next_sched_mod = bt_mesh_model_find(elem,
					BT_MESH_MODEL_ID_SCHEDULER_SRV);
		}

	} while (elem != NULL && next_sched_mod == NULL);

	uint8_t tmp_idx = srv->idx;

	srv->idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	schedule_action(srv, tmp_idx);
	run_scheduler(srv);
}

static void encode_status(struct bt_mesh_scheduler_srv *srv,
			  struct net_buf_simple *buf)
{
	bt_mesh_model_msg_init(buf, BT_MESH_SCHEDULER_OP_STATUS);
	net_buf_simple_add_le16(buf, srv->status_bitmap);
	BT_DBG("Tx: scheduler server status: %#2x", srv->status_bitmap);
}

static void encode_action_status(struct bt_mesh_scheduler_srv *srv,
				 struct net_buf_simple *buf,
				 uint8_t idx)
{
	bt_mesh_model_msg_init(buf, BT_MESH_SCHEDULER_OP_ACTION_STATUS);
	scheduler_action_pack(buf, idx, &srv->sch_reg[idx]);

	BT_DBG("Tx: scheduler server action status:");
	BT_DBG("              index: %d", idx);
	BT_DBG("               year: %d", srv->sch_reg[idx].year);
	BT_DBG("              month: %#2x", srv->sch_reg[idx].month);
	BT_DBG("                day: %d", srv->sch_reg[idx].day);
	BT_DBG("               wday: %#2x", srv->sch_reg[idx].day_of_week);
	BT_DBG("               hour: %d", srv->sch_reg[idx].hour);
	BT_DBG("             minute: %d", srv->sch_reg[idx].minute);
	BT_DBG("             second: %d", srv->sch_reg[idx].second);
	BT_DBG("             action: %d", srv->sch_reg[idx].action);
	BT_DBG("         transition: %d", srv->sch_reg[idx].transition_time);
	BT_DBG("              scene: %d", srv->sch_reg[idx].scene_number);
}

static int send_scheduler_status(struct bt_mesh_model *model,
		struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_STATUS,
			BT_MESH_SCHEDULER_MSG_LEN_STATUS);
	encode_status(srv, &buf);

	return model_send(model, ctx, &buf);
}

static int send_scheduler_action_status(struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					uint8_t idx)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_STATUS,
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS);

	encode_action_status(srv, &buf, idx);

	return model_send(model, ctx, &buf);
}

static void action_set(struct bt_mesh_model *model,
		       struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf,
		       bool ack)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;
	uint8_t idx;
	struct bt_mesh_schedule_entry tmp;

	scheduler_action_unpack(buf, &idx, &tmp);

	/* check against prohibited values */
	if (tmp.year > BT_MESH_SCHEDULER_ANY_YEAR ||
	    tmp.day > MAX_DAY ||
	    tmp.hour > BT_MESH_SCHEDULER_ONCE_A_DAY ||
	    tmp.minute > BT_MESH_SCHEDULER_ONCE_AN_HOUR ||
	    tmp.second > BT_MESH_SCHEDULER_ONCE_A_MINUTE ||
	    (tmp.action > BT_MESH_SCHEDULER_SCENE_RECALL &&
			    tmp.action != BT_MESH_SCHEDULER_NO_ACTIONS) ||
	    idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return;
	}

	srv->sch_reg[idx] = tmp;
	BT_DBG("Rx: scheduler server action index %d set, ack %d", idx, ack);

	if (srv->sch_reg[idx].action < BT_MESH_SCHEDULER_SCENE_RECALL ||
	   (srv->sch_reg[idx].action == BT_MESH_SCHEDULER_SCENE_RECALL &&
	    srv->sch_reg[idx].scene_number != 0)) {
		schedule_action(srv, idx);
		run_scheduler(srv);
	}

	if (srv->action_set_cb) {
		srv->action_set_cb(srv, ctx, idx, &srv->sch_reg[idx]);
	}

	/* publish state changing */
	send_scheduler_action_status(model, NULL, idx);

	if (ack) { /* reply on the action set command */
		send_scheduler_action_status(model, ctx, idx);
	}
}

static void handle_scheduler_get(struct bt_mesh_model *mod,
				 struct bt_mesh_msg_ctx *ctx,
				 struct net_buf_simple *buf)
{
	BT_DBG("Rx: scheduler server get");
	send_scheduler_status(mod, ctx);
}

static void handle_scheduler_action_get(struct bt_mesh_model *mod,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	uint8_t idx = net_buf_simple_pull_u8(buf);

	if (idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return;
	}

	BT_DBG("Rx: scheduler server action index %d get", idx);
	send_scheduler_action_status(mod, ctx, idx);
}

static void handle_scheduler_action_set(struct bt_mesh_model *mod,
					struct bt_mesh_msg_ctx *ctx,
					struct net_buf_simple *buf)
{
	action_set(mod, ctx, buf, true);
}

static void handle_scheduler_action_set_unack(struct bt_mesh_model *mod,
					      struct bt_mesh_msg_ctx *ctx,
					      struct net_buf_simple *buf)
{
	action_set(mod, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_scheduler_srv_op[] = {
	{ BT_MESH_SCHEDULER_OP_GET,
	  BT_MESH_SCHEDULER_MSG_LEN_GET,
	  handle_scheduler_get },
	{ BT_MESH_SCHEDULER_OP_ACTION_GET,
	  BT_MESH_SCHEDULER_MSG_LEN_ACTION_GET,
	  handle_scheduler_action_get },
	  BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_scheduler_setup_srv_op[] = {
	{ BT_MESH_SCHEDULER_OP_ACTION_SET,
	  BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET,
	  handle_scheduler_action_set },
	{ BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
	  BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET,
	  handle_scheduler_action_set_unack },
	  BT_MESH_MODEL_OP_END,
};

static int update_handler(struct bt_mesh_model *model)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	encode_status(srv, srv->pub.msg);
	return 0;
}

static int scheduler_srv_init(struct bt_mesh_model *mod)
{
	struct bt_mesh_scheduler_srv *srv = mod->user_data;

	if (srv->time_srv == NULL) {
		return -ECANCELED;
	}

	srv->mod = mod;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
			sizeof(srv->pub_data));
	srv->status_bitmap = 0;
	memset(&srv->sch_reg, 0, sizeof(srv->sch_reg));

	if (IS_ENABLED(CONFIG_BT_MESH_MODEL_EXTENSIONS)) {
		/* Model extensions:
		 * To simplify the model extension tree, we're flipping the
		 * relationship between the scheduler server and the scheduler
		 * setup server. In the specification, the scheduler setup
		 * server extends the scheduler server, which is the opposite
		 * of what we're doing here. This makes no difference for
		 * the mesh stack, but it makes it a lot easier to extend
		 * this model, as we won't have to support multiple extenders.
		 */
		bt_mesh_model_extend(mod, srv->setup_mod);

		/* The Scheduler Server is a main model that extends
		 * the Scene Server model.
		 */
		bt_mesh_model_extend(srv->scene_srv.mod, mod);

		/* The Scheduler Setup Server is a main model that extends
		 * the Scene Setup Server model.
		 */
		bt_mesh_model_extend(srv->scene_srv.setup_mod, srv->setup_mod);

		/* The Scheduler Setup Server is a main model that extends
		 * the Generic Power OnOff Setup Server.
		 */
		struct bt_mesh_model *gen_ponoff_setup_mod =
			bt_mesh_model_find(bt_mesh_model_elem(mod),
				BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SETUP_SRV);

		if (gen_ponoff_setup_mod) {
			bt_mesh_model_extend(gen_ponoff_setup_mod,
					srv->setup_mod);
		}
	}

	srv->idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	srv->status_bitmap = 0;
	k_delayed_work_init(&srv->delayed_work, scheduled_action_handle);

	for (int i = 0; i < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT; i++) {
		srv->sched_tai[i].sec = 0;
		srv->sched_tai[i].subsec = 0;
	}

	return 0;
}

static void scheduler_srv_reset(struct bt_mesh_model *mod)
{
	struct bt_mesh_scheduler_srv *srv = mod->user_data;

	srv->idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	srv->status_bitmap = 0;
	k_delayed_work_cancel(&srv->delayed_work);
	net_buf_simple_reset(srv->pub.msg);
}

const struct bt_mesh_model_cb _bt_mesh_scheduler_srv_cb = {
	.init = scheduler_srv_init,
	.reset = scheduler_srv_reset,
};

int bt_mesh_scheduler_srv_time_update(struct bt_mesh_scheduler_srv *srv)
{
	if (srv == NULL) {
		return -EINVAL;
	}

	run_scheduler(srv);
	return 0;
}
