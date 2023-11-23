/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <bluetooth/mesh/models.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/random/random.h>
#include "model_utils.h"
#include "time_util.h"
#include "scheduler_internal.h"
#include "mesh/net.h"
#include "mesh/access.h"

#define LOG_LEVEL CONFIG_BT_MESH_MODEL_LOG_LEVEL
#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(bt_mesh_scheduler_srv);

#define JANUARY           0
#define DECEMBER         11

enum {
	YEAR_STAGE,
	MONTH_STAGE,
	DAY_STAGE,
	HOUR_STAGE,
	MINUTE_STAGE,
	SECOND_STAGE,
	PROTECTOR_STAGE,
	FINAL_STAGE,
	ERROR_STAGE
};

struct tm_converter {
	bool consider_ovflw;
	int start_year;
	int start_month;
	int start_day;
	int start_hour;
	int start_minute;
	int start_second;
};

typedef int (*stage_handler_t)(struct tm *sched_time,
		struct tm *current_local,
		struct bt_mesh_schedule_entry *entry,
		struct tm_converter *info);

static int store(struct bt_mesh_scheduler_srv *srv, uint8_t idx, bool store_ndel)
{
	char name[3] = {0};
	const void *data = store_ndel ? &srv->sch_reg[idx] : NULL;
	size_t len = store_ndel ? sizeof(srv->sch_reg[idx]) : 0;

	(void) snprintf(name, sizeof(name), "%x", idx);

	return bt_mesh_model_data_store(srv->model, false, name, data, len);
}

static bool is_entry_defined(struct bt_mesh_scheduler_srv *srv, uint8_t idx)
{
	return srv->sch_reg[idx].action != BT_MESH_SCHEDULER_NO_ACTIONS;
}

static int get_days_in_month(int year, int month)
{
	int days[12] = {31, is_leap_year(year) ? 29 : 28,
		31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	return days[month];
}

static int get_day_of_week(int year, int month, int day)
{
	int day_cnt = 0;

	year += TM_START_YEAR;

	for (int i = TM_START_YEAR; i < year; i++) {
		day_cnt += is_leap_year(i) ? DAYS_LEAP_YEAR : DAYS_YEAR;
	}

	for (int i = 0; i < month; i++) {
		day_cnt += get_days_in_month(year, i);
	}

	day_cnt += day;
	return (day_cnt - 1) % WEEKDAY_CNT;
}

static bool day_validation(struct tm *sched_time, int day)
{
	return day <= get_days_in_month(sched_time->tm_year + TM_START_YEAR,
			sched_time->tm_mon);
}

static int year_handler(struct tm *sched_time, struct tm *current_local,
		struct bt_mesh_schedule_entry *entry, struct tm_converter *info)
{
	if (info->start_year != current_local->tm_year &&
		entry->year != BT_MESH_SCHEDULER_ANY_YEAR) {
		return ERROR_STAGE;
	}

	uint8_t current_year = info->start_year % 100;
	uint8_t diff = entry->year >= current_year ?
		entry->year - current_year : 100 - current_year + entry->year;

	sched_time->tm_year = entry->year == BT_MESH_SCHEDULER_ANY_YEAR ?
			info->start_year : info->start_year + diff;

	info->start_month = sched_time->tm_year == current_local->tm_year ?
			current_local->tm_mon : 0;

	return MONTH_STAGE;
}

static int month_handler(struct tm *sched_time, struct tm *current_local,
		struct bt_mesh_schedule_entry *entry, struct tm_converter *info)
{
	int month = entry->month;
	month &= (BIT_MASK(12) << info->start_month);
	if (month == 0) {
		info->start_year++;
		return YEAR_STAGE;
	}

	sched_time->tm_mon = u32_count_trailing_zeros(month);

	info->consider_ovflw = sched_time->tm_mon == current_local->tm_mon &&
			sched_time->tm_year == current_local->tm_year;
	info->start_day = info->consider_ovflw ? current_local->tm_mday : 1;

	return DAY_STAGE;
}

static int day_handler(struct tm *sched_time, struct tm *current_local,
		struct bt_mesh_schedule_entry *entry, struct tm_converter *info)
{
	bool day_ovflw = false;

	if (entry->day == BT_MESH_SCHEDULER_ANY_DAY) {
		if (!day_validation(sched_time, info->start_day)) {
			day_ovflw = true;
		}

		sched_time->tm_mday = info->start_day;
	} else {
		entry->day = MIN(entry->day,
			get_days_in_month(sched_time->tm_year + TM_START_YEAR,
					sched_time->tm_mon));

		sched_time->tm_mday = entry->day;
		if (sched_time->tm_mday < current_local->tm_mday) {
			day_ovflw = true;
		}
	}

	if (day_ovflw && info->consider_ovflw) {
		info->start_month++;
		return MONTH_STAGE;
	}

	sched_time->tm_wday = get_day_of_week(sched_time->tm_year,
			sched_time->tm_mon, sched_time->tm_mday);

	if (!(entry->day_of_week & (1 << sched_time->tm_wday))) {
		if (entry->day == BT_MESH_SCHEDULER_ANY_DAY) {
			int rest_wday = entry->day_of_week >> sched_time->tm_wday;
			int delta = rest_wday ? u32_count_trailing_zeros(rest_wday) :
				u32_count_trailing_zeros(entry->day_of_week) +
				WEEKDAY_CNT - sched_time->tm_wday;

			info->start_day += delta;
			sched_time->tm_mday = info->start_day;

			if (!day_validation(sched_time, info->start_day)) {
				day_ovflw = true;
			}
		} else {
			day_ovflw = true;
		}
	}

	if (day_ovflw && info->consider_ovflw) {
		info->start_month++;
		return MONTH_STAGE;
	}

	info->consider_ovflw = info->consider_ovflw &&
		sched_time->tm_mday == current_local->tm_mday;
	info->start_hour = info->consider_ovflw ? current_local->tm_hour : 0;

	return HOUR_STAGE;
}

static int hour_handler(struct tm *sched_time, struct tm *current_local,
		struct bt_mesh_schedule_entry *entry, struct tm_converter *info)
{
	bool hour_ovflw = false;

	if (entry->hour == BT_MESH_SCHEDULER_ONCE_A_DAY) {
		sched_time->tm_hour = sys_rand32_get() % 24;
		hour_ovflw = true;
	} else if (entry->hour == BT_MESH_SCHEDULER_ANY_HOUR) {
		hour_ovflw = info->start_hour > 23;
		sched_time->tm_hour = hour_ovflw ? 0 : info->start_hour;
	} else {
		hour_ovflw = entry->hour < info->start_hour;
		sched_time->tm_hour = entry->hour;
	}

	if (hour_ovflw && info->consider_ovflw) {
		info->start_day++;
		if (day_validation(sched_time, info->start_day)) {
			return DAY_STAGE;
		}

		info->start_month++;
		return MONTH_STAGE;
	}

	info->consider_ovflw = info->consider_ovflw &&
			sched_time->tm_hour == current_local->tm_hour;
	info->start_minute = info->consider_ovflw ? current_local->tm_min : 0;

	return MINUTE_STAGE;
}

static int minute_handler(struct tm *sched_time, struct tm *current_local,
		struct bt_mesh_schedule_entry *entry, struct tm_converter *info)
{
	bool minute_ovflw = false;

	if (entry->minute == BT_MESH_SCHEDULER_EVERY_15_MINUTES) {
		info->start_minute = 15 * DIV_ROUND_UP(current_local->tm_min + 1, 15);
		minute_ovflw = info->start_minute == 60;
		sched_time->tm_min = minute_ovflw ? 0 : info->start_minute;
	} else if (entry->minute == BT_MESH_SCHEDULER_EVERY_20_MINUTES) {
		info->start_minute = 20 * DIV_ROUND_UP(current_local->tm_min + 1, 20);
		minute_ovflw = info->start_minute == 60;
		sched_time->tm_min = minute_ovflw ? 0 : info->start_minute;
	} else if (entry->minute == BT_MESH_SCHEDULER_ONCE_AN_HOUR) {
		sched_time->tm_min = sys_rand32_get() % 60;
		minute_ovflw = true;
	} else if (entry->minute == BT_MESH_SCHEDULER_ANY_MINUTE) {
		minute_ovflw = info->start_minute > 59;
		sched_time->tm_min = minute_ovflw ? 0 : info->start_minute;
	} else {
		minute_ovflw = entry->minute < info->start_minute;
		sched_time->tm_min = entry->minute;
	}

	if (minute_ovflw && info->consider_ovflw) {
		info->start_hour++;
		return HOUR_STAGE;
	}

	info->consider_ovflw = info->consider_ovflw &&
			sched_time->tm_min == current_local->tm_min;
	info->start_second = info->consider_ovflw ? current_local->tm_sec : 0;

	return SECOND_STAGE;
}

static int second_handler(struct tm *sched_time, struct tm *current_local,
		struct bt_mesh_schedule_entry *entry, struct tm_converter *info)
{
	bool second_ovflw = false;

	if (entry->second == BT_MESH_SCHEDULER_EVERY_15_SECONDS) {
		info->start_second = 15 * DIV_ROUND_UP(current_local->tm_sec + 1, 15);
		second_ovflw = info->start_second == 60;
		sched_time->tm_sec = second_ovflw ? 0 : info->start_second;
	} else if (entry->second == BT_MESH_SCHEDULER_EVERY_20_SECONDS) {
		info->start_second = 20 * DIV_ROUND_UP(current_local->tm_sec + 1, 20);
		second_ovflw = info->start_second == 60;
		sched_time->tm_sec = second_ovflw ? 0 : info->start_second;
	} else if (entry->second == BT_MESH_SCHEDULER_ONCE_A_MINUTE) {
		sched_time->tm_sec = sys_rand32_get() % 60;
		second_ovflw = true;
	} else if (entry->second == BT_MESH_SCHEDULER_ANY_SECOND) {
		second_ovflw = info->start_second > 59;
		sched_time->tm_sec = second_ovflw ? 0 : info->start_second;
	} else {
		second_ovflw = entry->second < info->start_second;
		sched_time->tm_sec = entry->second;
	}

	if (second_ovflw && info->consider_ovflw) {
		info->start_minute++;
		return MINUTE_STAGE;
	}

	return PROTECTOR_STAGE;
}

static int protector_handler(struct tm *sched_time, struct tm *current_local,
		struct bt_mesh_schedule_entry *entry, struct tm_converter *info)
{
	/* prevent scheduling the fired action again */
	if (current_local->tm_year == sched_time->tm_year &&
		current_local->tm_mon == sched_time->tm_mon &&
		current_local->tm_mday == sched_time->tm_mday &&
		current_local->tm_hour == sched_time->tm_hour &&
		current_local->tm_min == sched_time->tm_min &&
		current_local->tm_sec == sched_time->tm_sec) {

		info->consider_ovflw = true;

		if (entry->second == BT_MESH_SCHEDULER_ANY_SECOND) {
			info->start_second++;
			return SECOND_STAGE;
		}

		if (entry->minute == BT_MESH_SCHEDULER_ANY_MINUTE) {
			info->start_minute++;
			return MINUTE_STAGE;
		}

		if (entry->hour == BT_MESH_SCHEDULER_ANY_HOUR) {
			info->start_hour++;
			return HOUR_STAGE;
		}

		if (entry->day == BT_MESH_SCHEDULER_ANY_DAY) {
			info->start_day++;
			return DAY_STAGE;
		}

		if (entry->year == BT_MESH_SCHEDULER_ANY_YEAR) {
			info->start_year++;
			return YEAR_STAGE;
		}

		return ERROR_STAGE;
	}

	return FINAL_STAGE;
}

static bool convert_scheduler_time_to_tm(struct tm *sched_time,
					struct tm *current_local,
					struct bt_mesh_schedule_entry *entry)
{
	int stage = YEAR_STAGE;
	struct tm_converter conv_info;
	const stage_handler_t handlers[] = {
		year_handler,
		month_handler,
		day_handler,
		hour_handler,
		minute_handler,
		second_handler,
		protector_handler
	};

	if (entry->month == 0) {
		return false;
	}

	if (entry->day_of_week == 0) {
		return false;
	}

	memset(&conv_info, 0, sizeof(struct tm_converter));
	conv_info.start_year = current_local->tm_year;

	while (stage != FINAL_STAGE && stage != ERROR_STAGE) {
		stage = handlers[stage](sched_time, current_local, entry, &conv_info);
	}

	return stage == FINAL_STAGE;
}

static uint8_t get_least_time_index(struct bt_mesh_scheduler_srv *srv)
{
	uint8_t cnt = u32_count_trailing_zeros(srv->active_bitmap);
	uint8_t idx = cnt;

	while (++cnt < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		if (srv->active_bitmap & BIT(cnt)) {
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

	tai_to_ts(&srv->sched_tai[planned_idx], &sched_time);
	int64_t scheduled_uptime = bt_mesh_time_srv_mktime(srv->time_srv,
			&sched_time);

	if (scheduled_uptime < 0) {
		LOG_WRN("Scheduler not started. Error: %lld.", scheduled_uptime);
		return;
	}

	srv->idx = planned_idx;
	k_work_reschedule(&srv->delayed_work,
			  K_MSEC(MAX(scheduled_uptime - current_uptime, 0)));
	LOG_DBG("Scheduler started. Target uptime: %lld. Current uptime: %lld.",
			scheduled_uptime, current_uptime);
}

static void schedule_action(struct bt_mesh_scheduler_srv *srv,
			    uint8_t idx)
{
	struct tm sched_time = {0};
	struct bt_mesh_schedule_entry *entry = &srv->sch_reg[idx];

	int64_t current_uptime = k_uptime_get();
	struct tm *current_local = bt_mesh_time_srv_localtime(srv->time_srv,
			current_uptime);

	if (current_local == NULL) {
		LOG_WRN("Local time not available");
		return;
	}

	LOG_DBG("Current uptime %lld", current_uptime);

	LOG_DBG("Current time:");
	LOG_DBG("        year: %d", current_local->tm_year);
	LOG_DBG("       month: %d", current_local->tm_mon);
	LOG_DBG("         day: %d", current_local->tm_mday);
	LOG_DBG("        hour: %d", current_local->tm_hour);
	LOG_DBG("      minute: %d", current_local->tm_min);
	LOG_DBG("      second: %d", current_local->tm_sec);

	if (!convert_scheduler_time_to_tm(&sched_time, current_local, entry)) {
		LOG_DBG("Cannot convert scheduled action time to struct tm");
		return;
	}

	if (ts_to_tai(&srv->sched_tai[idx], &sched_time)) {
		LOG_WRN("tm cannot be converted into TAI");
		return;
	}

	LOG_DBG("Scheduled time:");
	LOG_DBG("          year: %d", sched_time.tm_year);
	LOG_DBG("         month: %d", sched_time.tm_mon);
	LOG_DBG("           day: %d", sched_time.tm_mday);
	LOG_DBG("          hour: %d", sched_time.tm_hour);
	LOG_DBG("        minute: %d", sched_time.tm_min);
	LOG_DBG("        second: %d", sched_time.tm_sec);

	WRITE_BIT(srv->active_bitmap, idx, 1);
}

static void scheduled_action_handle(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct bt_mesh_scheduler_srv *srv = CONTAINER_OF(dwork,
				struct bt_mesh_scheduler_srv,
				delayed_work);

	if (srv->idx == BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		/* Disabled without cancelling timer */
		return;
	}

	WRITE_BIT(srv->active_bitmap, srv->idx, 0);

	struct bt_mesh_model *next_sched_mod = NULL;
	uint16_t model_id = srv->sch_reg[srv->idx].action ==
				BT_MESH_SCHEDULER_SCENE_RECALL ?
		BT_MESH_MODEL_ID_SCENE_SRV : BT_MESH_MODEL_ID_GEN_ONOFF_SRV;
	struct bt_mesh_model_transition transition = {
		.time = model_transition_decode(
				srv->sch_reg[srv->idx].transition_time),
		.delay = 0,
	};
	uint16_t scene = srv->sch_reg[srv->idx].scene_number;
	struct bt_mesh_elem *elem = bt_mesh_model_elem(srv->model);

	LOG_DBG("Scheduler action fired: %d", srv->sch_reg[srv->idx].action);

	do {
		struct bt_mesh_model *handled_model =
				bt_mesh_model_find(elem, model_id);

		if (model_id == BT_MESH_MODEL_ID_SCENE_SRV &&
		    handled_model != NULL) {
			struct bt_mesh_scene_srv *scene_srv =
			(struct bt_mesh_scene_srv *)handled_model->user_data;

			bt_mesh_scene_srv_set(scene_srv, scene, &transition);
			bt_mesh_scene_srv_pub(scene_srv, NULL);
			LOG_DBG("Scene srv addr: %d recalled scene: %d",
				elem->addr,
				srv->sch_reg[srv->idx].scene_number);
		}

		if (model_id == BT_MESH_MODEL_ID_GEN_ONOFF_SRV &&
		    handled_model != NULL) {
			struct bt_mesh_onoff_srv *onoff_srv =
			(struct bt_mesh_onoff_srv *)handled_model->user_data;
			struct bt_mesh_onoff_set set = {
				.on_off = srv->sch_reg[srv->idx].action,
				.transition = &transition
			};
			struct bt_mesh_onoff_status status = { 0 };

			onoff_srv->handlers->set(onoff_srv,
					NULL, &set, &status);
			bt_mesh_onoff_srv_pub(onoff_srv, NULL, &status);
			LOG_DBG("Onoff srv addr: %d set: %d",
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

	srv->last_idx = srv->idx;
	srv->idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	schedule_action(srv, srv->last_idx);
	run_scheduler(srv);
}

static void encode_status(struct bt_mesh_scheduler_srv *srv,
			  struct net_buf_simple *buf)
{
	bt_mesh_model_msg_init(buf, BT_MESH_SCHEDULER_OP_STATUS);

	uint16_t status_bitmap = 0;

	for (int i = 0; i < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT; i++) {
		if (is_entry_defined(srv, i)) {
			WRITE_BIT(status_bitmap, i, 1);
		}
	}

	net_buf_simple_add_le16(buf, status_bitmap);
	LOG_DBG("Tx: scheduler server status: %#2x", status_bitmap);
}

static void encode_action_status(struct bt_mesh_scheduler_srv *srv,
				 struct net_buf_simple *buf,
				 uint8_t idx,
				 bool is_reduced)
{
	bt_mesh_model_msg_init(buf, BT_MESH_SCHEDULER_OP_ACTION_STATUS);

	LOG_DBG("Tx: scheduler server action status:");
	LOG_DBG("        index: %d", idx);

	if (is_reduced) {
		net_buf_simple_add_u8(buf, idx);
	} else {
		scheduler_action_pack(buf, idx, &srv->sch_reg[idx]);

		LOG_DBG("         year: %d", srv->sch_reg[idx].year);
		LOG_DBG("        month: %#2x", srv->sch_reg[idx].month);
		LOG_DBG("          day: %d", srv->sch_reg[idx].day);
		LOG_DBG("         wday: %#2x", srv->sch_reg[idx].day_of_week);
		LOG_DBG("         hour: %d", srv->sch_reg[idx].hour);
		LOG_DBG("       minute: %d", srv->sch_reg[idx].minute);
		LOG_DBG("       second: %d", srv->sch_reg[idx].second);
		LOG_DBG("       action: %d", srv->sch_reg[idx].action);
		LOG_DBG("   transition: %d", srv->sch_reg[idx].transition_time);
		LOG_DBG("        scene: %d", srv->sch_reg[idx].scene_number);
	}
}

static int send_scheduler_status(const struct bt_mesh_model *model,
		struct bt_mesh_msg_ctx *ctx)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_STATUS,
			BT_MESH_SCHEDULER_MSG_LEN_STATUS);
	encode_status(srv, &buf);

	return bt_mesh_msg_send(model, ctx, &buf);
}

static int send_scheduler_action_status(const struct bt_mesh_model *model,
					struct bt_mesh_msg_ctx *ctx,
					uint8_t idx,
					bool is_reduced)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_SCHEDULER_OP_ACTION_STATUS,
			is_reduced ?
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS_REDUCED :
			BT_MESH_SCHEDULER_MSG_LEN_ACTION_STATUS);

	encode_action_status(srv, &buf, idx, is_reduced);

	return bt_mesh_msg_send(model, ctx, &buf);
}

static int action_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf, bool ack)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;
	uint8_t idx;
	struct bt_mesh_schedule_entry tmp = { 0 };

	scheduler_action_unpack(buf, &idx, &tmp);

	if (!scheduler_action_valid(&tmp, idx)) {
		return -EINVAL;
	}

	srv->sch_reg[idx] = tmp;
	LOG_DBG("Rx: scheduler server action index %d set, ack %d", idx, ack);

	if (srv->sch_reg[idx].action < BT_MESH_SCHEDULER_SCENE_RECALL ||
	   (srv->sch_reg[idx].action == BT_MESH_SCHEDULER_SCENE_RECALL &&
	    srv->sch_reg[idx].scene_number != 0)) {
		schedule_action(srv, idx);
		run_scheduler(srv);
	}

	if (srv->action_set_cb) {
		srv->action_set_cb(srv, ctx, idx, &srv->sch_reg[idx]);
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		store(srv, idx, is_entry_defined(srv, idx));
	}

	if (is_entry_defined(srv, idx)) {
		/* publish state changing */
		send_scheduler_action_status(model, NULL, idx, false);
	}

	if (ack) { /* reply on the action set command */
		send_scheduler_action_status(model, ctx, idx, false);
	}

	return 0;
}

static int handle_scheduler_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	LOG_DBG("Rx: scheduler server get");
	send_scheduler_status(model, ctx);

	return 0;
}

static int handle_scheduler_action_get(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;
	uint8_t idx = net_buf_simple_pull_u8(buf);

	if (idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return -ENOENT;
	}

	LOG_DBG("Rx: scheduler server action index %d get", idx);
	send_scheduler_action_status(model, ctx, idx,
			!is_entry_defined(srv, idx));

	return 0;
}

static int handle_scheduler_action_set(const struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
				       struct net_buf_simple *buf)
{
	return action_set(model, ctx, buf, true);
}

static int handle_scheduler_action_set_unack(const struct bt_mesh_model *model,
					     struct bt_mesh_msg_ctx *ctx,
					     struct net_buf_simple *buf)
{
	return action_set(model, ctx, buf, false);
}

const struct bt_mesh_model_op _bt_mesh_scheduler_srv_op[] = {
	{
		BT_MESH_SCHEDULER_OP_GET,
		BT_MESH_LEN_EXACT(BT_MESH_SCHEDULER_MSG_LEN_GET),
		handle_scheduler_get,
	},
	{
		BT_MESH_SCHEDULER_OP_ACTION_GET,
		BT_MESH_LEN_EXACT(BT_MESH_SCHEDULER_MSG_LEN_ACTION_GET),
		handle_scheduler_action_get,
	},
	BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op _bt_mesh_scheduler_setup_srv_op[] = {
	{
		BT_MESH_SCHEDULER_OP_ACTION_SET,
		BT_MESH_LEN_EXACT(BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET),
		handle_scheduler_action_set,
	},
	{
		BT_MESH_SCHEDULER_OP_ACTION_SET_UNACK,
		BT_MESH_LEN_EXACT(BT_MESH_SCHEDULER_MSG_LEN_ACTION_SET),
		handle_scheduler_action_set_unack,
	},
	BT_MESH_MODEL_OP_END,
};

static int update_handler(const struct bt_mesh_model *model)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	if (srv->last_idx == BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		encode_status(srv, srv->pub.msg);
	} else {
		encode_action_status(srv, srv->pub.msg, srv->last_idx, false);
	}
	return 0;
}

static int scheduler_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	if (srv->time_srv == NULL) {
		return -ECANCELED;
	}

	srv->model = model;
	srv->pub.msg = &srv->pub_buf;
	srv->pub.update = update_handler;
	net_buf_simple_init_with_data(&srv->pub_buf, srv->pub_data,
			sizeof(srv->pub_data));
	srv->active_bitmap = 0;

	srv->idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	srv->last_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	k_work_init_delayable(&srv->delayed_work, scheduled_action_handle);

	for (int i = 0; i < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT; i++) {
		srv->sch_reg[i].action = BT_MESH_SCHEDULER_NO_ACTIONS;
	}

	return 0;
}

static void scheduler_srv_reset(const struct bt_mesh_model *model)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;

	srv->idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	srv->last_idx = BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
	srv->active_bitmap = 0;
	/* If this cancellation fails, we'll exit early from the timer handler,
	 * as srv->idx is out of bounds.
	 */
	k_work_cancel_delayable(&srv->delayed_work);
	net_buf_simple_reset(srv->pub.msg);

	for (int i = 0; i < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT; i++) {
		srv->sch_reg[i].action = BT_MESH_SCHEDULER_NO_ACTIONS;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		for (int idx = 0; idx < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT;
		     ++idx) {
			store(srv, idx, false);
		}
	}
}

#ifdef CONFIG_BT_SETTINGS
static int scheduler_srv_settings_set(const struct bt_mesh_model *model,
				      const char *name,
				      size_t len_rd, settings_read_cb read_cb,
				      void *cb_data)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;
	struct bt_mesh_schedule_entry data = { 0 };
	ssize_t len = read_cb(cb_data, &data, sizeof(data));
	uint8_t idx = strtol(name, NULL, 16);

	if (len < sizeof(data) || idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return -EINVAL;
	}

	srv->sch_reg[idx] = data;

	return 0;
}
#endif

const struct bt_mesh_model_cb _bt_mesh_scheduler_srv_cb = {
	.init = scheduler_srv_init,
	.reset = scheduler_srv_reset,
#ifdef CONFIG_BT_SETTINGS
	.settings_set = scheduler_srv_settings_set
#endif
};

static int scheduler_setup_srv_init(const struct bt_mesh_model *model)
{
	struct bt_mesh_scheduler_srv *srv = model->user_data;
#if defined(CONFIG_BT_MESH_COMP_PAGE_1)
	int err = bt_mesh_model_correspond(model, srv->model);

	if (err) {
		return err;
	}
#endif

	return bt_mesh_model_extend(model, srv->model);
}

const struct bt_mesh_model_cb _bt_mesh_scheduler_setup_srv_cb = {
	.init = scheduler_setup_srv_init,
};

int bt_mesh_scheduler_srv_time_update(struct bt_mesh_scheduler_srv *srv)
{
	if (srv == NULL) {
		return -EINVAL;
	}

	for (int idx = 0; idx < BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT; ++idx) {
		schedule_action(srv, idx);
	}

	run_scheduler(srv);
	return 0;
}
