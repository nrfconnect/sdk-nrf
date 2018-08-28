/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <misc/dlist.h>
#include <misc/printk.h>
#include <logging/sys_log.h>
#include <event_manager.h>

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
			printk("e: %s ", et->name);
			if (et->print_event) {
				et->print_event(eh);
			}
			printk("\n");
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
					printk("|\t%s notified%s\n",
						el->name,
						(consumed)?(" (event consumed)"):(""));
				}
			}
		}
		trace_event_execution(eh, false);
		k_free(eh);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENTS) &&
	    IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_EVENT_HANDLERS)) {
		printk("|\n\n");
	}
}

void _event_submit(struct event_header *eh)
{
	unsigned int flags = irq_lock();

	sys_dlist_append(&eventq, &eh->node);

	irq_unlock(flags);

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_PROFILER_ENABLED)) {
		const struct event_type *et = eh->type_id;

		if (et->ev_info && et->ev_info->log_arg_fn) {
			struct log_event_buf buf;

			ARG_UNUSED(buf);
			profiler_log_start(&buf);
			if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
				profiler_log_add_mem_address(&buf, eh);
			}
			et->ev_info->log_arg_fn(&buf, eh);
			profiler_log_send(&buf,
			  profiler_event_ids[et - __start_event_types]);
		}
	}
	k_work_submit(&event_processor);
}

static void event_manager_show_listeners(void)
{
	printk("Registered Listeners:\n");
	for (const struct event_listener *el = __start_event_listeners;
	     el != __stop_event_listeners;
	     el++) {
		__ASSERT_NO_MSG(el != NULL);
		printk("|\t[L:%s]\n", el->name);
	}
	printk("|\n\n");
}

static void event_manager_show_subscribers(void)
{
	printk("Registered Subscribers:\n");
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
				printk("|\tprio:%u\t[E:%s] -> [L:%s]\n", prio,
						et->name, el->name);

				is_subscribed = true;
			}
		}

		if (!is_subscribed) {
			printk("|\t[E:%s] has no subscribers\n", et->name);
		}
		printk("|\n");
	}
	printk("\n");
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
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
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
			SYS_LOG_ERR("System profiler: "
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

