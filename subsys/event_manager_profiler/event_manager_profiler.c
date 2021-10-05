/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <event_manager.h>
#include <event_manager_profiler.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(event_manager_profiler, CONFIG_EVENT_MANAGER_LOG_LEVEL);

#define IDS_COUNT (CONFIG_EVENT_MANAGER_MAX_EVENT_CNT + 2)

extern const struct profiler_info __start_profiler_info[];
extern const struct profiler_info __stop_profiler_info[];

static uint16_t profiler_event_ids[IDS_COUNT];

void event_manager_trace_event_execution(const struct event_header *eh, bool is_start)
{
	size_t event_cnt = __stop_profiler_info - __start_profiler_info;
	size_t event_idx = event_cnt + (is_start ? 0 : 1);
	size_t trace_evt_id = profiler_event_ids[event_idx];

	if (!IS_ENABLED(CONFIG_EVENT_MANAGER_PROFILER_TRACE_EVENT_EXECUTION) ||
	    !is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;

	ARG_UNUSED(buf);

	profiler_log_start(&buf);
	profiler_log_add_mem_address(&buf, eh);
	profiler_log_send(&buf, trace_evt_id);
}

void event_manager_trace_event_submission(const struct event_header *eh, const void *profiler_info)
{
	if (!profiler_info) {
		return;
	}

	size_t event_idx = (const struct profiler_info *) profiler_info - __start_profiler_info;
	size_t trace_evt_id = profiler_event_ids[event_idx];

	if (!is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;

	ARG_UNUSED(buf);

	profiler_log_start(&buf);

	if (IS_ENABLED(CONFIG_EVENT_MANAGER_PROFILER_TRACE_EVENT_EXECUTION)) {
		profiler_log_add_mem_address(&buf, eh);
	}
	if (IS_ENABLED(CONFIG_EVENT_MANAGER_PROFILER_PROFILE_EVENT_DATA)) {
		((const struct profiler_info *)profiler_info)->profile_fn(&buf, eh);
	}

	profiler_log_send(&buf, trace_evt_id);
}

void trace_register_execution_tracking_events(void)
{
	static const char * const labels[] = {EM_MEM_ADDRESS_LABEL};
	enum profiler_arg types[] = {MEM_ADDRESS_TYPE};
	size_t event_cnt = __stop_profiler_info - __start_profiler_info;
	uint16_t profiler_event_id;

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
	for (const struct profiler_info *profiler_info = __start_profiler_info;
	     (profiler_info != __stop_profiler_info); profiler_info++) {
		size_t event_idx = profiler_info - __start_profiler_info;
		uint16_t profiler_event_id;

		profiler_event_id = profiler_register_event_type(
			profiler_info->name,
			profiler_info->profiler_arg_labels,
			profiler_info->profiler_arg_types,
			profiler_info->profiler_arg_cnt);
		profiler_event_ids[event_idx] = profiler_event_id;
	}

	if (IS_ENABLED(CONFIG_EVENT_MANAGER_PROFILER_TRACE_EVENT_EXECUTION)) {
		trace_register_execution_tracking_events();
	}
}

int event_manager_event_manager_trace_event_init(void)
{
	/* Every profiled Event Manager event registers a single profiler event.
	 * Apart from that 2 additional profiler events are used to indicate processing
	 * start and end of an Event Manager event.
	 */
	__ASSERT_NO_MSG(__stop_profiler_info - __start_profiler_info + 2 <=
			CONFIG_PROFILER_MAX_NUMBER_OF_EVENTS);

	if (profiler_init()) {
		LOG_ERR("System profiler: initialization problem\n");
		return -EFAULT;
	}
	trace_register_events();

	return 0;
}
