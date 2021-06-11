/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "mbed-trace/mbed_trace.h"
#include "mbed-trace-helper.h"

#define MODULE pelion_trace
#include <caf/events/module_state_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_PELION_CLIENT_PELION_LOG_LEVEL);


static void trace_printer(const char *str)
{
	LOG_INF("%s", log_strdup(str));
}

static void init_pelion_trace(void)
{
	bool ret;
	/* Create mutex for tracing to avoid broken lines in logs */
	ret = mbed_trace_helper_create_mutex();
	__ASSERT_NO_MSG(ret);
	(void) mbed_trace_init();
	/*
	 * If you want, you can filter out the trace set.
	 * For example  mbed_trace_include_filters_set("COAP");
	 * would enable only the COAP level traces.
	 * Or           mbed_trace_exclude_filters_set("COAP");
	 * would leave them out.
	 * More details on mbed_trace.h file.
	 */
	mbed_trace_mutex_wait_function_set(mbed_trace_helper_mutex_wait);
	mbed_trace_mutex_release_function_set(mbed_trace_helper_mutex_release);
	mbed_trace_config_set(TRACE_MODE_COLOR | TRACE_ACTIVE_LEVEL_ALL);
	mbed_trace_print_function_set(trace_printer);
}


static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			init_pelion_trace();
			return false;
		}
		return false;
	}

	/* Event not handled but subscribed. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
