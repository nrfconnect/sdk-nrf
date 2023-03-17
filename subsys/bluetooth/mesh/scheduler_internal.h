/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Scheduler model internal API
 */

#ifndef SCHEDULER_INTERNAL_H_
#define SCHEDULER_INTERNAL_H_

#include <bluetooth/mesh/scheduler.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_DAY 0x1F

static inline bool scheduler_action_valid(const struct bt_mesh_schedule_entry *entry, uint8_t idx)
{
	if (entry == NULL ||
	    entry->year > BT_MESH_SCHEDULER_ANY_YEAR ||
	    entry->day > MAX_DAY ||
	    entry->hour > BT_MESH_SCHEDULER_ONCE_A_DAY ||
	    entry->minute > BT_MESH_SCHEDULER_ONCE_AN_HOUR ||
	    entry->second > BT_MESH_SCHEDULER_ONCE_A_MINUTE ||
	    (entry->action > BT_MESH_SCHEDULER_SCENE_RECALL &&
	     entry->action != BT_MESH_SCHEDULER_NO_ACTIONS) ||
	    idx >= BT_MESH_SCHEDULER_ACTION_ENTRY_COUNT) {
		return false;
	}

	return true;
}

static inline void scheduler_action_unpack(struct net_buf_simple *buf,
					uint8_t *idx,
					struct bt_mesh_schedule_entry *entry)
{
	uint32_t word;

	word = net_buf_simple_pull_le32(buf);
	*idx = word & BIT_MASK(4);
	entry->year = (word >> 4) & BIT_MASK(7);
	entry->month = (word >> 11) & BIT_MASK(12);
	entry->day = (word >> 23) & BIT_MASK(5);
	entry->hour = (word >> 28);
	word = net_buf_simple_pull_le32(buf);
	entry->hour |=  (word & BIT_MASK(1)) << 4;
	entry->minute = (word >> 1) & BIT_MASK(6);
	entry->second = (word >> 7) & BIT_MASK(6);
	entry->day_of_week = (word >> 13) & BIT_MASK(7);
	entry->action = (word >> 20) & BIT_MASK(4);
	entry->transition_time = (word >> 24) & BIT_MASK(8);
	entry->scene_number = net_buf_simple_pull_le16(buf);
}

static inline void scheduler_action_pack(struct net_buf_simple *buf,
				uint8_t idx,
				const struct bt_mesh_schedule_entry *entry)
{
	net_buf_simple_add_le32(buf, ((idx & BIT_MASK(4)) |
				((entry->year & BIT_MASK(7)) << 4) |
				((entry->month & BIT_MASK(12)) << 11) |
				((entry->day & BIT_MASK(5)) << 23) |
				((entry->hour & BIT_MASK(5)) << 28)));
	net_buf_simple_add_le32(buf,
			(((entry->hour >> 4) & BIT_MASK(1)) |
			((entry->minute & BIT_MASK(6)) << 1) |
			((entry->second & BIT_MASK(6)) << 7) |
			((entry->day_of_week & BIT_MASK(7)) << 13) |
			((entry->action & BIT_MASK(4)) << 20) |
			((entry->transition_time & BIT_MASK(8)) << 24)));
	net_buf_simple_add_le16(buf, entry->scene_number);
}

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULER_INTERNAL_H_ */
