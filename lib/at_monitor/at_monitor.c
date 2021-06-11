/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stddef.h>
#include <string.h>
#include <zephyr.h>
#include <modem/at_monitor.h>
#include <toolchain/common.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(at_monitor);

struct at_notif {
	void *fifo_reserved;
	char data[];
};

static void at_monitor_task(struct k_work *work);

static K_WORK_DEFINE(at_monitor_work, at_monitor_task);
static K_HEAP_DEFINE(at_monitor_heap, CONFIG_AT_MONITOR_HEAP_SIZE);
static K_FIFO_DEFINE(at_monitor_fifo);

static void at_monitor_task(struct k_work *work)
{
	struct at_notif *at_notif;

	while ((at_notif = k_fifo_get(&at_monitor_fifo, K_NO_WAIT))) {
		/* Match notification with all monitors */
		Z_STRUCT_SECTION_FOREACH(at_monitor_entry, e) {
			if (!e->paused && (!e->notif || strstr(at_notif->data, e->notif))) {
				LOG_INF("Dispatching: %s", at_notif->data);
				e->handler(at_notif->data);
			}
		}
		k_heap_free(&at_monitor_heap, at_notif);
	}
}

void at_monitor_dispatch(const char *notif)
{
	struct at_notif *at_notif;
	size_t sz_needed;

	sz_needed = sizeof(struct at_notif) + strlen(notif) + sizeof(char);

	at_notif = k_heap_alloc(&at_monitor_heap, sz_needed, K_NO_WAIT);
	if (!at_notif) {
		LOG_WRN("No heap space for incoming notification: %s",
			log_strdup(notif));
		return;
	}

	memset(at_notif, 0x00, sz_needed);
	strcpy(at_notif->data, notif);

	k_fifo_put(&at_monitor_fifo, at_notif);
	k_work_submit(&at_monitor_work);
}
