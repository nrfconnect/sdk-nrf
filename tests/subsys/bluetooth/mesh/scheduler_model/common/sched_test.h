/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <scheduler_internal.h>
#include <time_util.h>

#define DELTA_TIME   1
#define SUBSEC_STEPS 256

/* item of struct tm */
#define TM_INIT(year, month, day, hour, minute, second) \
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

void expected_tm_check(struct tm *received, struct tm *expected, int steps);
void start_time_adjust(struct tm *start_tm);
