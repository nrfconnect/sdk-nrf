/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "ctrl_events.h"

#include <zephyr.h>
#include <kernel.h>
#include <errno.h>

#include "macros_common.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ctrl_events, CONFIG_LOG_CTRL_EVENTS_LEVEL);

#define CTRL_EVENTS_MSGQ_MAX_ELEMENTS 5
#define CTRL_EVENTS_MSGQ_ALIGNMENT_WORDS 4

K_MSGQ_DEFINE(msg_queue, sizeof(struct event_t), CTRL_EVENTS_MSGQ_MAX_ELEMENTS,
	      CTRL_EVENTS_MSGQ_ALIGNMENT_WORDS);

bool ctrl_events_queue_empty(void)
{
	uint32_t num_free;

	num_free = k_msgq_num_free_get(&msg_queue);

	return (num_free == CTRL_EVENTS_MSGQ_MAX_ELEMENTS ? true : false);
}

int ctrl_events_put(struct event_t *event)
{
	if (event == NULL) {
		LOG_ERR("Event is NULL. Event source: %d", event->event_source);
		return -EFAULT;
	}

	return k_msgq_put(&msg_queue, (void *)event, K_NO_WAIT);
}

int ctrl_events_get(struct event_t *my_event, k_timeout_t timeout)
{
	return k_msgq_get(&msg_queue, (void *)my_event, timeout);
}
