/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stddef.h>
#include <string.h>
#include <zephyr.h>
#include <init.h>
#include <device.h>
#include <nrf_modem_at.h>
#include <modem/at_monitor.h>
#include <toolchain/common.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(at_monitor);

struct at_notif_fifo {
	void *fifo_reserved;
	char data[];
};

static void at_monitor_task(struct k_work *work);

static K_FIFO_DEFINE(at_monitor_fifo);
static K_HEAP_DEFINE(at_monitor_heap, CONFIG_AT_MONITOR_HEAP_SIZE);
static K_WORK_DEFINE(at_monitor_work, at_monitor_task);

static void at_monitor_dispatch(const char *notif)
{
	bool monitored;
	struct at_notif_fifo *at_notif;
	size_t sz_needed;

	/* Dispatch to monitors in ISR, if any.
	 * There might be non-ISR monitors that are interested
	 * in the same notification, so copy it to the heap afterwards regardless.
	 */
	STRUCT_SECTION_FOREACH(at_monitor_isr_entry, e) {
		if (!e->paused && (e->filter == ANY || strstr(notif, e->filter))) {
			LOG_DBG("Dispatching to %p (ISR)", e->handler);
			e->handler(notif);
		}
	}

	monitored = false;
	STRUCT_SECTION_FOREACH(at_monitor_entry, e) {
		if (!e->paused && (e->filter == ANY || strstr(notif, e->filter))) {
			monitored = true;
			break;
		}
	}

	if (!monitored) {
		return;
	}

	sz_needed = sizeof(struct at_notif_fifo) + strlen(notif) + sizeof(char);

	at_notif = k_heap_alloc(&at_monitor_heap, sz_needed, K_NO_WAIT);
	if (!at_notif) {
		LOG_WRN("No heap space for incoming notification: %s",
			log_strdup(notif));
		return;
	}

	strcpy(at_notif->data, notif);

	k_fifo_put(&at_monitor_fifo, at_notif);
	k_work_submit(&at_monitor_work);
}

static void at_monitor_task(struct k_work *work)
{
	struct at_notif_fifo *at_notif;

	while ((at_notif = k_fifo_get(&at_monitor_fifo, K_NO_WAIT))) {
		/* Match notification with all monitors */
		LOG_DBG("AT notif: %s", log_strdup(at_notif->data));
		STRUCT_SECTION_FOREACH(at_monitor_entry, e) {
			if (!e->paused &&
			   (e->filter == ANY || strstr(at_notif->data, e->filter))) {
				LOG_DBG("Dispatching to %p", e->handler);
				e->handler(at_notif->data);
			}
		}
		k_heap_free(&at_monitor_heap, at_notif);
	}
}

static int at_monitor_sys_init(const struct device *unused)
{
	int err;

	err = nrf_modem_at_notif_handler_set(at_monitor_dispatch);
	if (err) {
		LOG_ERR("Failed to hook the dispatch function, err %d", err);
	}

	return 0;
}

#if defined(CONFIG_AT_MONITOR_SYS_INIT)
/* Initialize during SYS_INIT */
SYS_INIT(at_monitor_sys_init, APPLICATION, 0);
#endif

void at_monitor_init(void)
{
	at_monitor_sys_init(NULL);
}
