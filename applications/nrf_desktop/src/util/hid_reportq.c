/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <zephyr/sys/slist.h>
#include <zephyr/kernel.h>

#include "hid_reportq.h"
#include "hid_report_desc.h"
#include "hid_event.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hid_reportq, CONFIG_DESKTOP_HID_REPORTQ_LOG_LEVEL);

#define MAX_ENQUEUED_REPORTS CONFIG_DESKTOP_HID_REPORTQ_MAX_ENQUEUED_REPORTS

struct enqueued_report {
	sys_snode_t node;
	struct hid_report_event *event;
};

struct counted_list {
	sys_slist_t list;
	size_t node_count;
};

struct hid_reportq {
	struct counted_list report_lists[ARRAY_SIZE(input_reports)];
	uint16_t enabled_report_idx_bm;
	uint8_t last_sent_report_idx;
	uint8_t report_max;
	uint8_t report_cnt;
	const void *sub_id;
};

static struct hid_reportq queues[CONFIG_DESKTOP_HID_REPORTQ_QUEUE_COUNT];

/* Ensure that enabled_report_idx_bm can handle all of the report indexes. */
BUILD_ASSERT(ARRAY_SIZE(input_reports) <= 16);

static struct enqueued_report *get_enqueued_report(struct counted_list *cnt_list)
{
	sys_snode_t *node = sys_slist_get(&cnt_list->list);

	if (!node) {
		return NULL;
	}

	__ASSERT_NO_MSG(cnt_list->node_count > 0);
	cnt_list->node_count--;

	return CONTAINER_OF(node, struct enqueued_report, node);
}

static void enqueue_report(struct counted_list *cnt_list, struct enqueued_report *report)
{
	__ASSERT_NO_MSG(cnt_list->node_count < MAX_ENQUEUED_REPORTS);
	cnt_list->node_count++;

	sys_slist_append(&cnt_list->list, &report->node);
}

static struct hid_report_event *get_enqueued_event(struct counted_list *cnt_list)
{
	struct enqueued_report *report = get_enqueued_report(cnt_list);

	if (!report) {
		return NULL;
	}

	struct hid_report_event *event = report->event;

	k_free(report);

	return event;
}

static void drop_enqueued_events(struct counted_list *cnt_list)
{
	struct hid_report_event *event = get_enqueued_event(cnt_list);

	while (event) {
		app_event_manager_free(event);
		event = get_enqueued_event(cnt_list);
	}

	__ASSERT_NO_MSG(cnt_list->node_count == 0);
}

static void enqueue_event(struct counted_list *cnt_list, struct hid_report_event *event)
{
	struct enqueued_report *report;

	if (cnt_list->node_count < MAX_ENQUEUED_REPORTS) {
		report = k_malloc(sizeof(*report));
	} else {
		LOG_WRN("Enqueue dropped the oldest report");

		report = get_enqueued_report(cnt_list);
		__ASSERT_NO_MSG(report);
		app_event_manager_free(report->event);
	}

	if (!report) {
		LOG_ERR("Cannot allocate enqueued_report");
		/* Should never happen. */
		__ASSERT_NO_MSG(false);
	} else {
		report->event = event;
		enqueue_report(cnt_list, report);
	}
}

static struct hid_reportq *reportq_find_free(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(queues); i++) {
		struct hid_reportq *q = &queues[i];

		if (!q->sub_id) {
			return q;
		}
	}

	return NULL;
}

struct hid_reportq *hid_reportq_alloc(const void *sub_id, uint8_t report_max)
{
	__ASSERT_NO_MSG(sub_id);
	__ASSERT_NO_MSG(report_max > 0);

	struct hid_reportq *q = reportq_find_free();

	if (!q) {
		return NULL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(q->report_lists); i++) {
		__ASSERT_NO_MSG(q->report_lists[i].node_count == 0);
		sys_slist_init(&q->report_lists[i].list);
	}

	__ASSERT_NO_MSG(q->enabled_report_idx_bm == 0);
	__ASSERT_NO_MSG(q->last_sent_report_idx == 0);
	__ASSERT_NO_MSG(q->report_max == 0);
	__ASSERT_NO_MSG(q->report_cnt == 0);

	q->sub_id = sub_id;
	q->report_max = report_max;

	return q;
}

void hid_reportq_free(struct hid_reportq *q)
{
	/* Make sure that queue was allocated. */
	__ASSERT_NO_MSG(q->sub_id);

	for (size_t i = 0; i < ARRAY_SIZE(q->report_lists); i++) {
		drop_enqueued_events(&q->report_lists[i]);
	}

	q->enabled_report_idx_bm = 0;
	q->last_sent_report_idx = 0;
	q->report_max = 0;
	q->report_cnt = 0;
	q->sub_id = NULL;
}

const void *hid_reportq_get_sub_id(struct hid_reportq *q)
{
	/* Make sure that queue was allocated. */
	__ASSERT_NO_MSG(q->sub_id);

	return q->sub_id;
}

static uint8_t get_input_report_idx(uint8_t rep_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(input_reports); i++) {
		__ASSERT_NO_MSG(i <= UINT8_MAX);

		if (input_reports[i] == rep_id) {
			return i;
		}
	}

	/* Should not happen. */
	__ASSERT_NO_MSG(false);

	BUILD_ASSERT(ARRAY_SIZE(input_reports) <= UINT8_MAX);
	return ARRAY_SIZE(input_reports);
}

int hid_reportq_report_add(struct hid_reportq *q, const void *src_id, uint8_t rep_id,
			   const uint8_t *data, size_t size)
{
	/* Make sure that queue was allocated. */
	__ASSERT_NO_MSG(q->sub_id);

	uint8_t rep_idx = get_input_report_idx(rep_id);

	if (!hid_reportq_is_subscribed(q, rep_id)) {
		return -EACCES;
	}

	struct hid_report_event *event = new_hid_report_event(sizeof(rep_id) + size);

	event->source = src_id;
	event->subscriber = q->sub_id;

	/* Forward report as is adding report id on the front. */
	event->dyndata.data[0] = rep_id;
	memcpy(&event->dyndata.data[1], data, size);

	if (q->report_cnt < q->report_max) {
		APP_EVENT_SUBMIT(event);
		q->last_sent_report_idx = rep_idx;
		q->report_cnt++;
	} else {
		enqueue_event(&q->report_lists[rep_idx], event);
	}

	return 0;
}

static struct hid_report_event *get_next_enqueued_event(struct hid_reportq *q)
{
	uint8_t rep_idx = q->last_sent_report_idx;
	struct hid_report_event *event;

	do {
		rep_idx = (rep_idx + 1) % ARRAY_SIZE(q->report_lists);

		event = get_enqueued_event(&q->report_lists[rep_idx]);
		if (event) {
			q->last_sent_report_idx = rep_idx;
			return event;
		}
	} while (rep_idx != q->last_sent_report_idx);

	return get_enqueued_event(&q->report_lists[rep_idx]);
}

void hid_reportq_report_sent(struct hid_reportq *q, uint8_t rep_id, bool err)
{
	ARG_UNUSED(rep_id);
	ARG_UNUSED(err);

	/* Make sure that queue was allocated. */
	__ASSERT_NO_MSG(q->sub_id);

	struct hid_report_event *event = get_next_enqueued_event(q);

	if (event) {
		APP_EVENT_SUBMIT(event);
	} else {
		q->report_cnt--;
	}
}

bool hid_reportq_is_subscribed(struct hid_reportq *q, uint8_t rep_id)
{
	/* Make sure that queue was allocated. */
	__ASSERT_NO_MSG(q->sub_id);

	uint8_t rep_idx = get_input_report_idx(rep_id);

	return ((q->enabled_report_idx_bm & BIT(rep_idx)) != 0);
}

void hid_reportq_subscribe(struct hid_reportq *q, uint8_t rep_id)
{
	/* Make sure that queue was allocated. */
	__ASSERT_NO_MSG(q->sub_id);

	uint8_t rep_idx = get_input_report_idx(rep_id);

	WRITE_BIT(q->enabled_report_idx_bm, rep_idx, 1);
	__ASSERT_NO_MSG(q->report_lists[rep_idx].node_count == 0);
}

void hid_reportq_unsubscribe(struct hid_reportq *q, uint8_t rep_id)
{
	/* Make sure that queue was allocated. */
	__ASSERT_NO_MSG(q->sub_id);

	uint8_t rep_idx = get_input_report_idx(rep_id);

	WRITE_BIT(q->enabled_report_idx_bm, rep_idx, 0);
	drop_enqueued_events(&q->report_lists[rep_idx]);
}
