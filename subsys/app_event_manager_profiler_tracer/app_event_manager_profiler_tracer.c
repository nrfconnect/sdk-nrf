/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(app_event_manager_profiler_tracer, CONFIG_APP_EVENT_MANAGER_LOG_LEVEL);

#define IDS_COUNT (CONFIG_APP_EVENT_MANAGER_MAX_EVENT_CNT + 2)

extern struct profiler_info _profiler_info_list_start[];
extern struct profiler_info _profiler_info_list_end[];

static uint16_t profiler_event_ids[IDS_COUNT];

/** @brief Trace event execution.
 *
 * @param aeh        Pointer to the event header of the event that is processed by app_event_manager.
 * @param is_start  Bool value indicating if this occurrence is related
 *                  to start or end of the event processing.
 **/
static void app_event_manager_trace_event_execution(const struct application_event_header *aeh,
					      bool is_start)
{
	size_t event_cnt = _profiler_info_list_end - _profiler_info_list_start;
	size_t event_idx = event_cnt + (is_start ? 0 : 1);
	size_t trace_evt_id = profiler_event_ids[event_idx];

	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION) ||
	    !is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;

	ARG_UNUSED(buf);

	profiler_log_start(&buf);
	profiler_log_add_mem_address(&buf, aeh);
	profiler_log_send(&buf, trace_evt_id);
}

static void app_event_manager_trace_event_preprocess(const struct application_event_header *aeh)
{
	app_event_manager_trace_event_execution(aeh, true);
}

static void app_event_manager_trace_event_postprocess(const struct application_event_header *aeh)
{
	app_event_manager_trace_event_execution(aeh, false);
}

APPLICATION_EVENT_HOOK_PREPROCESS_REGISTER_FIRST(app_event_manager_trace_event_preprocess);
APPLICATION_EVENT_HOOK_POSTPROCESS_REGISTER_LAST(app_event_manager_trace_event_postprocess);

/** @brief Trace event submission.
 *
 * @param aeh Pointer to the event header of the event that is submitted to app_event_manager.
 **/
static void app_event_manager_trace_event_submission(const struct application_event_header *aeh)
{
	const void *profiler_info = aeh->type_id->trace_data;

	if (!profiler_info) {
		return;
	}

	size_t event_idx = (const struct profiler_info *) profiler_info - _profiler_info_list_start;
	size_t trace_evt_id = profiler_event_ids[event_idx];

	if (!is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;

	ARG_UNUSED(buf);

	profiler_log_start(&buf);

	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION)) {
		profiler_log_add_mem_address(&buf, aeh);
	}
	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_PROFILE_EVENT_DATA)) {
		((const struct profiler_info *)profiler_info)->profile_fn(&buf, aeh);
	}

	profiler_log_send(&buf, trace_evt_id);
}

APPLICATION_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST(app_event_manager_trace_event_submission);

static void trace_register_execution_tracking_events(void)
{
	static const char * const labels[] = {EM_MEM_ADDRESS_LABEL};
	enum profiler_arg types[] = {MEM_ADDRESS_TYPE};
	size_t event_cnt = _profiler_info_list_end - _profiler_info_list_start;
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
	STRUCT_SECTION_FOREACH(profiler_info, pi) {
		size_t event_idx = pi - _profiler_info_list_start;
		uint16_t profiler_event_id;

		profiler_event_id = profiler_register_event_type(
			pi->name,
			pi->profiler_arg_labels,
			pi->profiler_arg_types,
			pi->profiler_arg_cnt);
		profiler_event_ids[event_idx] = profiler_event_id;
	}

	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION)) {
		trace_register_execution_tracking_events();
	}
}

/** @brief Initialize tracing in the Application Event Manager.
 *
 * @retval 0 If the operation was successful.
 * @retval other Error code
 **/
static int app_event_manager_trace_event_init(void)
{
	/* Every profiled Application Event Manager event registers a single profiler event.
	 * Apart from that 2 additional profiler events are used to indicate processing
	 * start and end of an Application Event Manager event.
	 */
	__ASSERT_NO_MSG(_profiler_info_list_end - _profiler_info_list_start + 2 <=
			CONFIG_PROFILER_MAX_NUMBER_OF_APPLICATION_EVENTS);

	if (profiler_init()) {
		LOG_ERR("System profiler: initialization problem\n");
		return -EFAULT;
	}
	trace_register_events();

	return 0;
}

APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER(app_event_manager_trace_event_init);
