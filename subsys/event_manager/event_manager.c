/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <misc/dlist.h>
#include <misc/printk.h>

#include <event_manager.h>


static void event_processor_fn(struct k_work *work);

K_WORK_DEFINE(event_processor, event_processor_fn);

static sys_dlist_t eventq = SYS_DLIST_STATIC_INIT(&eventq);


static void event_processor_fn(struct k_work *work)
{
	sys_dlist_t events;

	/* Make current event list local */
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


	/* Traverse the list of events */
	struct event_header *eh;

	while (NULL != (eh = CONTAINER_OF(sys_dlist_get(&events),
					  struct event_header,
					  node))) {

		ASSERT_EVENT_ID(eh->type_id);

		const struct event_type *et = eh->type_id;

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

int event_manager_init(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_SHOW_LISTENERS)) {
		event_manager_show_listeners();
		event_manager_show_subscribers();
	}

	return 0;
}
