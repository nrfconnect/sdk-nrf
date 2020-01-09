/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <zephyr.h>
#include <spinlock.h>
#include <sys/slist.h>
#include <event_manager.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(event_manager, CONFIG_DESKTOP_EVENT_MANAGER_LOG_LEVEL);


static void event_processor_fn(struct k_work *work);


#if CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED
#define IDS_COUNT CONFIG_DESKTOP_EVENT_MANAGER_MAX_EVENT_CNT
#else
#define IDS_COUNT 0
#endif

#ifdef CONFIG_SHELL
extern u32_t event_manager_displayed_events;
#else
static u32_t event_manager_displayed_events;
#endif

static u16_t profiler_event_ids[IDS_COUNT];
static K_WORK_DEFINE(event_processor, event_processor_fn);
static sys_slist_t eventq = SYS_SLIST_STATIC_INIT(&eventq);
static struct k_spinlock lock;


static bool log_is_event_displayed(const struct event_type *et)
{
	u32_t event_mask = BIT(et - __start_event_types);
	return event_manager_displayed_events & event_mask;
}

static void log_event(const struct event_header *eh)
{
	const struct event_type *et = eh->type_id;

	if (!IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS) ||
	    !log_is_event_displayed(et)) {
		return;
	}

	if (et->log_event) {
		char log_buf[CONFIG_DESKTOP_EVENT_MANAGER_EVENT_LOG_BUF_LEN];

		int pos = et->log_event(eh, log_buf, sizeof(log_buf));

		if (pos < 0) {
			log_buf[0] = '\0';
		} else if (pos >= sizeof(log_buf)) {
			BUILD_ASSERT_MSG(sizeof(log_buf) >= 2,
					 "Buffer invalid");
			log_buf[sizeof(log_buf) - 2] = '~';
		}

		LOG_INF("e: %s %s", et->name, log_strdup(log_buf));
	} else {
		LOG_INF("e: %s", et->name);
	}
}

static void log_event_progress(const struct event_type *et,
			       const struct event_listener *el)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS) ||
	    !IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENT_HANDLERS) ||
	    !log_is_event_displayed(et)) {
		return;
	}

	LOG_INF("|\tnotifying %s", el->name);
}

static void log_event_consumed(const struct event_type *et)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS) ||
	    !IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENT_HANDLERS) ||
	    !log_is_event_displayed(et)) {
		return;
	}

	LOG_INF("|\tevent consumed");
}

static void log_event_init(void)
{
	if (!IS_ENABLED(CONFIG_LOG)) {
		return;
	}

	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {
		if (et->init_log_enable) {
			u32_t event_mask = BIT(et - __start_event_types);
			event_manager_displayed_events |= event_mask;
		}
	}
}

static void trace_event_execution(const struct event_header *eh, bool is_start)
{
	size_t event_cnt = __stop_event_types - __start_event_types;
	size_t event_idx = event_cnt + (is_start ? 0 : 1);
	size_t trace_evt_id = profiler_event_ids[event_idx];

	if (!IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION) ||
	    !is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;
	ARG_UNUSED(buf);

	profiler_log_start(&buf);
	profiler_log_add_mem_address(&buf, eh);
	profiler_log_send(&buf, trace_evt_id);
}

static void trace_event_submission(const struct event_header *eh)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED)) {
		return;
	}

	const struct event_type *et = eh->type_id;
	size_t event_idx = et - __start_event_types;
	size_t trace_evt_id = profiler_event_ids[event_idx];

	if (!et->ev_info ||
	    !et->ev_info->profile_fn ||
	    !is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;
	ARG_UNUSED(buf);

	profiler_log_start(&buf);

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
		profiler_log_add_mem_address(&buf, eh);
	}
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILE_EVENT_DATA)) {
		et->ev_info->profile_fn(&buf, eh);
	}

	profiler_log_send(&buf, trace_evt_id);
}

static void trace_register_execution_tracking_events(void)
{
	const char *labels[] = {"mem_address"};
	enum profiler_arg types[] = {PROFILER_ARG_U32};
	size_t event_cnt = __stop_event_types - __start_event_types;
	u16_t profiler_event_id;

	ARG_UNUSED(types);
	ARG_UNUSED(labels);

	/* Event execution start event after last event. */
	profiler_event_id = profiler_register_event_type(
				"event_processing_start",
				labels, types, 1);
	profiler_event_ids[event_cnt] = profiler_event_id;

	/* Event execution end event. */
	profiler_event_id = profiler_register_event_type(
				"event_processing_end",
				labels, types, 1);
	profiler_event_ids[event_cnt + 1] = profiler_event_id;
}

static void trace_register_events(void)
{
	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {
		if (et->ev_info) {
			size_t event_idx = et - __start_event_types;
			u16_t profiler_event_id;

			profiler_event_id = profiler_register_event_type(
				et->name, et->ev_info->log_arg_labels,
				et->ev_info->log_arg_types,
				et->ev_info->log_arg_cnt);
			profiler_event_ids[event_idx] = profiler_event_id;
		}
	}

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
		trace_register_execution_tracking_events();
	}
}

static int trace_event_init(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED)) {
		if (profiler_init()) {
			LOG_ERR("System profiler: "
				"initialization problem\n");
			return -EFAULT;
		}
		trace_register_events();
	}

	return 0;
}

static void event_processor_fn(struct k_work *work)
{
	sys_slist_t events = SYS_SLIST_STATIC_INIT(&events);

	/* Make current event list local. */
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (sys_slist_is_empty(&eventq)) {
		k_spin_unlock(&lock, key);
		return;
	}

	sys_slist_merge_slist(&events, &eventq);

	k_spin_unlock(&lock, key);


	/* Traverse the list of events. */
	sys_snode_t *node;
	while (NULL != (node = sys_slist_get(&events))) {
		struct event_header *eh = CONTAINER_OF(node,
						       struct event_header,
						       node);

		ASSERT_EVENT_ID(eh->type_id);

		const struct event_type *et = eh->type_id;

		trace_event_execution(eh, true);

		log_event(eh);

		bool consumed = false;

		for (size_t prio = SUBS_PRIO_MIN;
		     (prio <= SUBS_PRIO_MAX) && !consumed;
		     prio++) {
			for (const struct event_subscriber *es =
					et->subs_start[prio];
			     (es != et->subs_stop[prio]) && !consumed;
			     es++) {

				__ASSERT_NO_MSG(es != NULL);

				const struct event_listener *el = es->listener;

				__ASSERT_NO_MSG(el != NULL);
				__ASSERT_NO_MSG(el->notification != NULL);

				log_event_progress(et, el);

				consumed = el->notification(eh);

				if (consumed) {
					log_event_consumed(et);
				}
			}
		}

		trace_event_execution(eh, false);

		k_free(eh);
	}
}

void _event_submit(struct event_header *eh)
{
	__ASSERT_NO_MSG(eh);
	ASSERT_EVENT_ID(eh->type_id);

	trace_event_submission(eh);

	k_spinlock_key_t key = k_spin_lock(&lock);
	sys_slist_append(&eventq, &eh->node);
	k_spin_unlock(&lock, key);

	k_work_submit(&event_processor);
}

int event_manager_init(void)
{
	log_event_init();

	return trace_event_init();
}
