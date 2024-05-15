/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stddef.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <nrf_modem_at.h>
#include <modem/at_monitor.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at_monitor, CONFIG_AT_MONITOR_LOG_LEVEL);

struct at_notif_fifo {
	void *fifo_reserved;
	char data[]; /* Null-terminated AT notification string */
};

/* Workqueue task to dispatch notifications */
static void at_monitor_task(struct k_work *work);
static K_WORK_DEFINE(at_monitor_work, at_monitor_task);

/* Workqueue to dispatch notifications from */
#if defined(CONFIG_AT_MONITOR_USE_DEDICATED_WORKQUEUE)
static K_THREAD_STACK_DEFINE(at_monitor_work_q_stack, CONFIG_AT_MONITOR_WORKQ_STACK_SIZE);
static struct k_work_q at_monitor_work_q;
#else
extern struct k_work_q k_sys_work_q;
#endif

static K_FIFO_DEFINE(at_monitor_fifo);
static K_HEAP_DEFINE(at_monitor_heap, CONFIG_AT_MONITOR_HEAP_SIZE);

static bool is_paused(const struct at_monitor_entry *mon)
{
	return mon->flags.paused;
}

static bool is_direct(const struct at_monitor_entry *mon)
{
	return mon->flags.direct;
}

static bool has_match(const struct at_monitor_entry *mon, const char *notif)
{
	return (mon->filter == ANY || strstr(notif, mon->filter));
}

/* Dispatch AT notifications immediately, or schedules a workqueue task to do that.
 * Keep this function public so that it can be called by tests.
 * This function is called from an ISR.
 */
void at_monitor_dispatch(const char *notif)
{
	bool monitored;
	struct at_notif_fifo *at_notif;
	size_t sz_needed;

	__ASSERT_NO_MSG(notif != NULL);

	monitored = false;
	STRUCT_SECTION_FOREACH(at_monitor_entry, e) {
		if (!is_paused(e) && has_match(e, notif)) {
			if (is_direct(e)) {
				LOG_DBG("Dispatching to %p (ISR)", e->handler);
				e->handler(notif);
			} else {
				/* Copy and schedule work-queue task */
				monitored = true;
			}
		}
	}

	if (!monitored) {
		/* Only copy monitored notifications to save heap */
		return;
	}

	sz_needed = sizeof(struct at_notif_fifo) + strlen(notif) + sizeof(char);

	at_notif = k_heap_alloc(&at_monitor_heap, sz_needed, K_NO_WAIT);
	if (!at_notif) {
		LOG_WRN("No heap space for incoming notification: %s", notif);
		__ASSERT(at_notif, "No heap space for incoming notification: %s", notif);
		return;
	}

	strcpy(at_notif->data, notif);

	k_fifo_put(&at_monitor_fifo, at_notif);

	/* Dispatch to queue */
#if defined(CONFIG_AT_MONITOR_USE_DEDICATED_WORKQUEUE)
	k_work_submit_to_queue(&at_monitor_work_q, &at_monitor_work);
#else
	k_work_submit(&at_monitor_work);
#endif
}

static void at_monitor_task(struct k_work *work)
{
	struct at_notif_fifo *at_notif;

	while ((at_notif = k_fifo_get(&at_monitor_fifo, K_NO_WAIT))) {
		/* Match notification with all monitors */
		LOG_DBG("AT notif: %.*s", strlen(at_notif->data) - strlen("\r\n"), at_notif->data);
		STRUCT_SECTION_FOREACH(at_monitor_entry, e) {
			if (!is_paused(e) && !is_direct(e) && has_match(e, at_notif->data)) {
				LOG_DBG("Dispatching to %p", e->handler);
				e->handler(at_notif->data);
			}
		}
		k_heap_free(&at_monitor_heap, at_notif);
	}
}

static int at_monitor_sys_init(void)
{
	int err;

#if defined(CONFIG_AT_MONITOR_USE_DEDICATED_WORKQUEUE)
	/* Default to a cooperative priority to avoid being preempted */
#define THREAD_PRIORITY                                                                            \
	COND_CODE_1(CONFIG_AT_MONITOR_WORKQ_PRIO_OVERRIDE, (CONFIG_AT_MONITOR_WORKQ_PRIO), (-1))

	const struct k_work_queue_config cfg = {.name = "at_monitor_work_q"};

	k_work_queue_start(&at_monitor_work_q,
			   at_monitor_work_q_stack,
			   K_THREAD_STACK_SIZEOF(at_monitor_work_q_stack),
			   THREAD_PRIORITY, &cfg);
#endif

	err = nrf_modem_at_notif_handler_set(at_monitor_dispatch);
	if (err) {
		LOG_ERR("Failed to hook the dispatch function, err %d", err);
	}

	return 0;
}

/* Initialize during SYS_INIT */
SYS_INIT(at_monitor_sys_init, APPLICATION, 0);
