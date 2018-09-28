/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "button_event.h"

static void print_event(const struct event_header *eh)
{
	struct button_event *event = cast_button_event(eh);

	printk("key_id=0x%x %s",
			event->key_id,
			(event->pressed)?("pressed"):("released"));
}

static void log_args(struct log_event_buf *buf, const struct event_header *eh)
{
	struct button_event *event = cast_button_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->key_id);
	profiler_log_encode_u32(buf, (event->pressed)?(1):(0));
}

EVENT_INFO_DEFINE(button_event, ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U32),
			ENCODE("button_id", "status"), log_args);

EVENT_TYPE_DEFINE(button_event, print_event, &button_event_info);
