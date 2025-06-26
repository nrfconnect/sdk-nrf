/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "hid_eventq.h"

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hid_eventq, CONFIG_DESKTOP_HID_EVENTQ_LOG_LEVEL);

struct hid_eventq_data {
	uint16_t key_id;
	bool pressed;
};

struct hid_eventq_event {
	sys_snode_t node;
	struct hid_eventq_data data;
	int64_t timestamp;
};


static bool hid_eventq_is_initialized(const struct hid_eventq *q)
{
	__ASSERT_NO_MSG(q);

	return (q->cnt_max != 0);
}

void hid_eventq_init(struct hid_eventq *q, uint16_t max_queued)
{
	LOG_DBG("q:%p, max_queued:%" PRIu16, (void *)q, max_queued);

	ARG_UNUSED(hid_eventq_is_initialized);
	__ASSERT_NO_MSG(!hid_eventq_is_initialized(q));
	__ASSERT_NO_MSG(max_queued > 0);

	sys_slist_init(&q->root);
	q->cnt = 0;
	q->cnt_max = max_queued;
}

bool hid_eventq_is_full(const struct hid_eventq *q)
{
	__ASSERT_NO_MSG(hid_eventq_is_initialized(q));
	__ASSERT_NO_MSG(q->cnt <= q->cnt_max);

	return (q->cnt == q->cnt_max);
}

bool hid_eventq_is_empty(const struct hid_eventq *q)
{
	__ASSERT_NO_MSG(hid_eventq_is_initialized(q));
	__ASSERT_NO_MSG(q->cnt <= q->cnt_max);

	return (q->cnt == 0);
}

static void drop_oldest_hid_events(struct hid_eventq *q)
{
	LOG_DBG("q:%p", (void *)q);

	__ASSERT_NO_MSG(hid_eventq_is_full(q));

	sys_snode_t *i;

	SYS_SLIST_FOR_EACH_NODE(&q->root, i) {
		/* Try to remove events but only if key release was generated for each removed key
		 * press.
		 */
		int64_t node_timestamp = CONTAINER_OF(i, struct hid_eventq_event, node)->timestamp;

		/* Use incremented event timestamp to drop the event. */
		hid_eventq_cleanup(q, node_timestamp + 1);
		if (!hid_eventq_is_full(q)) {
			/* At least one element was removed from the queue.
			 * Do not continue list traverse, content was modified!
			 */
			break;
		}
	}
}

int hid_eventq_keypress_enqueue(struct hid_eventq *q, uint16_t id, bool pressed, bool drop_oldest)
{
	if (hid_eventq_is_full(q)) {
		if (drop_oldest) {
			/* Try to drop the oldest keypress events to make space for the new one. */
			drop_oldest_hid_events(q);
		}

		/* Return an error if queue is still full. */
		if (hid_eventq_is_full(q)) {
			return -ENOBUFS;
		}
	}

	struct hid_eventq_event *evt = k_malloc(sizeof(*evt));

	if (!evt) {
		LOG_ERR("hid_eventq_event allocation failed");
		return -ENOMEM;
	}

	evt->timestamp = k_uptime_get();
	evt->data.key_id = id;
	evt->data.pressed = pressed;

	LOG_DBG("q:%p, ts:%" PRId64 ", id:%" PRIu16 ", %s",
		(void *)q, evt->timestamp, id, pressed ? "press" : "release");

	/* Add a new event to the queue. */
	sys_slist_append(&q->root, &evt->node);
	q->cnt++;

	return 0;
}

int hid_eventq_keypress_dequeue(struct hid_eventq *q, uint16_t *id, bool *pressed)
{
	__ASSERT_NO_MSG(hid_eventq_is_initialized(q));
	__ASSERT_NO_MSG(id);
	__ASSERT_NO_MSG(pressed);

	sys_snode_t *n = sys_slist_get(&q->root);

	if (!n) {
		return -ENOENT;
	}

	struct hid_eventq_event *evt = CONTAINER_OF(n, struct hid_eventq_event, node);

	*id = evt->data.key_id;
	*pressed = evt->data.pressed;

	LOG_DBG("q:%p, ts:%" PRId64 ", id:%" PRIu16 ", %s",
		(void *)q, evt->timestamp, *id, *pressed ? "press" : "release");

	k_free(evt);
	q->cnt--;

	return 0;
}

static void hid_eventq_region_purge(struct hid_eventq *q, sys_snode_t *last_to_purge)
{
	sys_snode_t *tmp;
	sys_snode_t *tmp_safe;
	size_t removed_cnt = 0;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&q->root, tmp, tmp_safe) {
		__ASSERT_NO_MSG(tmp);

		sys_slist_remove(&q->root, NULL, tmp);
		k_free(CONTAINER_OF(tmp, struct hid_eventq_event, node));
		removed_cnt++;

		if (tmp == last_to_purge) {
			break;
		}
	}

	__ASSERT_NO_MSG(q->cnt >= removed_cnt);
	q->cnt -= removed_cnt;

	if (removed_cnt > 0) {
		LOG_WRN("%u stale events removed from the queue %p", removed_cnt, (void *)q);
	}
}

void hid_eventq_reset(struct hid_eventq *q)
{
	__ASSERT_NO_MSG(hid_eventq_is_initialized(q));

	LOG_DBG("q:%p", (void *)q);

	hid_eventq_region_purge(q, NULL);

	__ASSERT_NO_MSG(q->cnt == 0);
	__ASSERT_NO_MSG(sys_slist_is_empty(&q->root));
}

static sys_snode_t *get_first_valid_node(struct hid_eventq *q, int64_t min_timestamp)
{
	sys_snode_t *i;

	SYS_SLIST_FOR_EACH_NODE(&q->root, i) {
		struct hid_eventq_event *evt = CONTAINER_OF(i, struct hid_eventq_event, node);

		if (evt->timestamp >= min_timestamp) {
			return i;
		}
	}

	return NULL;
}

static sys_snode_t *get_keypress_release_node(struct hid_eventq *q, struct hid_eventq_event *evt,
					      int *evt_pos, sys_snode_t *limit)
{
	__ASSERT_NO_MSG(evt->data.pressed);

	size_t hit_count = 1;
	size_t pos_shift = 0;
	sys_snode_t *i = &evt->node;

	SYS_SLIST_ITERATE_FROM_NODE(&q->root, i) {
		pos_shift++;

		if (i == limit) {
			break;
		}

		const struct hid_eventq_event *cur = CONTAINER_OF(i, struct hid_eventq_event, node);

		if (cur->data.key_id == evt->data.key_id) {
			hit_count += cur->data.pressed ? (1) : (-1);

			if (hit_count == 0) {
				*evt_pos += pos_shift;

				/* Found matching keypress releases. */
				return i;
			}
		}
	}

	/* Not found. */
	return NULL;
}

void hid_eventq_cleanup(struct hid_eventq *q, int64_t min_timestamp)
{
	__ASSERT_NO_MSG(hid_eventq_is_initialized(q));

	LOG_DBG("q:%p, min_timestamp:%" PRId64, (void *)q, min_timestamp);

	sys_snode_t *first_valid = get_first_valid_node(q, min_timestamp);

	sys_snode_t *max_node = NULL;
	int max_pos = -1;

	sys_snode_t *cur_node;
	int cur_pos = 0;

	sys_snode_t *tmp_safe;

	/* Remove events but only if key release was generated for each removed key press. */
	SYS_SLIST_FOR_EACH_NODE_SAFE(&q->root, cur_node, tmp_safe) {
		if (cur_node == first_valid) {
			break;
		}

		struct hid_eventq_event *cur_evt =
			CONTAINER_OF(cur_node, struct hid_eventq_event, node);

		if (cur_evt->data.pressed) {
			int release_pos = cur_pos;
			sys_snode_t *release_node = get_keypress_release_node(q, cur_evt,
									      &release_pos,
									      first_valid);

			if (release_node) {
				if (release_pos > max_pos) {
					max_node = release_node;
					max_pos = release_pos;
				}
			} else {
				/* Node not found. Abort cleanup. */
				break;
			}
		} else {
			if (cur_pos > max_pos) {
				max_node = cur_node;
				max_pos = cur_pos;
			}
		}

		if (cur_node == max_node) {
			/* All keypresses up to this point have pairs and can be deleted. */
			hid_eventq_region_purge(q, max_node);
		}

		cur_pos++;
	}
}
