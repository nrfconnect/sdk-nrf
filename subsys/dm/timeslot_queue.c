/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "timeslot_queue.h"
#include "time.h"

#define TIMESLOT_QUEUE_LENGTH            CONFIG_DM_TIMESLOT_QUEUE_LENGTH
#define TIMESLOT_QUEUE_COUNT_SAME_PEER   CONFIG_DM_TIMESLOT_QUEUE_COUNT_SAME_PEER

#define MIN_TIME_BETWEEN_TIMESLOTS_US    CONFIG_DM_MIN_TIME_BETWEEN_TIMESLOTS_US
#define RANGING_OFFSET_US                CONFIG_DM_RANGING_OFFSET_US

static K_MUTEX_DEFINE(list_mtx);
static sys_slist_t timeslot_list = SYS_SLIST_STATIC_INIT(&timeslot_list);

struct timeslot_entry {
	struct timeslot_request timeslot_req;
	sys_snode_t node;
};

static K_HEAP_DEFINE(heap, TIMESLOT_QUEUE_LENGTH * sizeof(struct timeslot_entry));

static void list_lock(void)
{
	k_mutex_lock(&list_mtx, K_FOREVER);
}

static void list_unlock(void)
{
	k_mutex_unlock(&list_mtx);
}

static size_t get_list_size(void)
{
	size_t ret = 0;
	sys_snode_t *node, *tmp;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&timeslot_list, node, tmp) {
		ret++;
	}

	return ret;
}

static bool is_request_exist(struct dm_request *req)
{
	uint8_t cnt = 0;
	sys_snode_t *node, *tmp;
	struct timeslot_entry *item;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&timeslot_list, node, tmp) {
		item = CONTAINER_OF(node, struct timeslot_entry, node);
		if (bt_addr_le_cmp(&item->timeslot_req.dm_req.bt_addr, &req->bt_addr) == 0) {
			cnt++;
		}
	}

	return cnt >= TIMESLOT_QUEUE_COUNT_SAME_PEER;
}

int timeslot_queue_append(struct dm_request *req, uint32_t start_ref_tick,
			  uint32_t window_len_us, uint32_t timeslot_len_us)
{
	uint32_t distance;
	uint32_t start_time;
	uint32_t delay;
	size_t list_size;
	struct timeslot_entry *last, *item;

	delay = req->start_delay_us + RANGING_OFFSET_US;
	start_time = (start_ref_tick + US_TO_RTC_TICKS(delay)) % RTC_COUNTER_MAX;

	list_size = get_list_size();
	if (list_size >= TIMESLOT_QUEUE_LENGTH) {
		return -ENOMEM;
	}

	if (list_size != 0) {
		if (is_request_exist(req)) {
			return -EAGAIN;
		}

		list_lock();
		last = SYS_SLIST_PEEK_TAIL_CONTAINER(&timeslot_list, last, node);
		list_unlock();

		distance = time_distance_get(last->timeslot_req.start_time, start_time);
		if (distance < US_TO_RTC_TICKS(last->timeslot_req.timeslot_length_us +
					       MIN_TIME_BETWEEN_TIMESLOTS_US)) {
			return -EBUSY;
		}
	}

	item = k_heap_alloc(&heap, sizeof(struct timeslot_entry), K_NO_WAIT);
	if (!item) {
		return -ENOMEM;
	}

	item->timeslot_req.start_time = start_time;
	item->timeslot_req.timeslot_length_us = timeslot_len_us;
	item->timeslot_req.window_length_us = window_len_us;
	req->rng_seed++;

	memcpy(&item->timeslot_req.dm_req, req, sizeof(item->timeslot_req.dm_req));

	list_lock();
	sys_slist_append(&timeslot_list, &item->node);
	list_unlock();

	return 0;
}

struct timeslot_request *timeslot_queue_peek(void)
{
	struct timeslot_entry *item;

	list_lock();
	item = SYS_SLIST_PEEK_HEAD_CONTAINER(&timeslot_list, item, node);
	list_unlock();

	if (!item) {
		return NULL;
	}

	return &item->timeslot_req;
}

void timeslot_queue_remove_first(void)
{
	sys_snode_t *node;
	struct timeslot_entry *item;

	list_lock();
	node = sys_slist_get(&timeslot_list);
	list_unlock();

	if (!node) {
		return;
	}

	item = CONTAINER_OF(node, struct timeslot_entry, node);
	k_heap_free(&heap, item);
}
