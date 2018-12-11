/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <zephyr.h>
#include <misc/dlist.h>
#include <event_manager.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(event_manager, CONFIG_DESKTOP_EVENT_MANAGER_LOG_LEVEL);

static void event_processor_fn(struct k_work *work);
static void trace_event_execution(const struct event_header *eh,
				  bool is_start);

#if CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED
#define IDS_COUNT CONFIG_DESKTOP_EVENT_MANAGER_MAX_EVENT_CNT
#else
#define IDS_COUNT 0
#endif

static u16_t profiler_event_ids[IDS_COUNT];

K_WORK_DEFINE(event_processor, event_processor_fn);

static sys_dlist_t eventq = SYS_DLIST_STATIC_INIT(&eventq);

static void event_processor_fn(struct k_work *work)
{
	sys_dlist_t events;

	/* Make current event list local. */
	unsigned int flags = irq_lock();

	if (sys_dlist_is_empty(&eventq)) {
		irq_unlock(flags);
		return;
	}

	events = eventq;
	events.next->prev = &events;
	events.prev->next = &events;
	sys_dlist_init(&eventq);

	irq_unlock(flags);


	/* Traverse the list of events. */
	struct event_header *eh;

	while (NULL != (eh = CONTAINER_OF(sys_dlist_get(&events),
					  struct event_header,
					  node))) {

		ASSERT_EVENT_ID(eh->type_id);

		const struct event_type *et = eh->type_id;

		trace_event_execution(eh, true);
		if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS)) {
			if (et->log_event) {
				const size_t buf_len = CONFIG_DESKTOP_EVENT_MANAGER_EVENT_LOG_BUF_LEN;
				char event_log_buf[buf_len];
				int pos;

				pos = et->log_event(eh, event_log_buf, buf_len);
				if (pos < 0) {
					event_log_buf[0] = '\0';
				}
				if (pos >= buf_len) {
					event_log_buf[buf_len - 2] = '~';
				}
				LOG_INF("e: %s %s", et->name,
					log_strdup(event_log_buf));
			} else {
				LOG_INF("e: %s", et->name);
			}
		}

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

				consumed = el->notification(eh);

				if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS) &&
				    IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENT_HANDLERS)) {
					LOG_INF("|\t%s notified%s",
						el->name,
						(consumed)?(" (event consumed)"):(""));
				}
			}
		}
		trace_event_execution(eh, false);
		k_free(eh);
	}
}

void _event_submit(struct event_header *eh)
{
	unsigned int flags = irq_lock();

	sys_dlist_append(&eventq, &eh->node);

	irq_unlock(flags);

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED)) {
		const struct event_type *et = eh->type_id;

		if (et->ev_info && et->ev_info->profile_fn &&
		    is_profiling_enabled(profiler_event_ids[et - __start_event_types])) {
			struct log_event_buf buf;

			ARG_UNUSED(buf);
			profiler_log_start(&buf);
			if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
				profiler_log_add_mem_address(&buf, eh);
			}
			et->ev_info->profile_fn(&buf, eh);
			profiler_log_send(&buf,
			  profiler_event_ids[et - __start_event_types]);
		}
	}
	k_work_submit(&event_processor);
}

static void event_manager_show_listeners(void)
{
	LOG_INF("Registered Listeners:");
	for (const struct event_listener *el = __start_event_listeners;
	     el != __stop_event_listeners;
	     el++) {
		__ASSERT_NO_MSG(el != NULL);
		LOG_INF("|\t[L:%s]", el->name);
	}
	LOG_INF("");
}

static void event_manager_show_subscribers(void)
{
	LOG_INF("Registered Subscribers:");
	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {

		bool is_subscribed = false;

		for (size_t prio = SUBS_PRIO_MIN;
		     prio <= SUBS_PRIO_MAX;
		     prio++) {
			for (const struct event_subscriber *es =
					et->subs_start[prio];
			     es != et->subs_stop[prio];
			     es++) {

				__ASSERT_NO_MSG(es != NULL);
				const struct event_listener *el = es->listener;

				__ASSERT_NO_MSG(el != NULL);
				LOG_INF("|\tprio:%u\t[E:%s] -> [L:%s]", prio,
					et->name, el->name);

				is_subscribed = true;
			}
		}

		if (!is_subscribed) {
			LOG_INF("|\t[E:%s] has no subscribers", et->name);
		}
		LOG_INF("");
	}
	LOG_INF("");
}

static void register_execution_tracking_events(void)
{
	const enum profiler_arg types[] = {PROFILER_ARG_U32};
	const char *labels[] = {"mem_address"};
	u16_t profiler_event_id;

	ARG_UNUSED(types);
	ARG_UNUSED(labels);

	/* Event execution start event after last event. */
	profiler_event_id = profiler_register_event_type(
				"event_processing_start",
				labels, types, 1);
	profiler_event_ids[__stop_event_types - __start_event_types] =
							profiler_event_id;

	/* Event execution end event. */
	profiler_event_id = profiler_register_event_type(
				"event_processing_end",
				labels, types, 1);
	profiler_event_ids[__stop_event_types - __start_event_types + 1] =
							 profiler_event_id;
}

static void register_events(void)
{
	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {
		if (et->ev_info) {
			u16_t profiler_event_id;

			profiler_event_id = profiler_register_event_type(
				et->name, et->ev_info->log_arg_labels,
				et->ev_info->log_arg_types,
				et->ev_info->log_arg_cnt);
			profiler_event_ids[et - __start_event_types] =
						    profiler_event_id;
		}
	}
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
		register_execution_tracking_events();
	}
}

static void trace_event_execution(const struct event_header *eh, bool is_start)
{
	size_t tracing_event_id = profiler_event_ids[__stop_event_types -
						     __start_event_types +
						     (is_start ? 0 : 1)];

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION) &&
	    is_profiling_enabled(tracing_event_id)) {
		struct log_event_buf buf;

		ARG_UNUSED(buf);
		profiler_log_start(&buf);
		profiler_log_add_mem_address(&buf, eh);
		/* Event execution end in event manager has next id
		 * after all the events and event execution start.
		 */
		profiler_log_send(&buf, profiler_event_ids[__stop_event_types -
		__start_event_types + (is_start ? 0 : 1)]);
	}
}

int event_manager_init(void)
{

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED)) {
		if (profiler_init()) {
			LOG_ERR("System profiler: "
				"initialization problem\n");
			return -1;
		}
		register_events();
	}

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_LISTENERS)) {
		event_manager_show_listeners();
		event_manager_show_subscribers();
	}
	return 0;
}

