/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_event_manager_profiler_tracer, CONFIG_APP_EVENT_MANAGER_LOG_LEVEL);

#define IDS_COUNT (CONFIG_APP_EVENT_MANAGER_MAX_EVENT_CNT + 2)

extern struct nrf_profiler_info _nrf_profiler_info_list_start[];
extern struct nrf_profiler_info _nrf_profiler_info_list_end[];

static uint16_t nrf_profiler_event_ids[IDS_COUNT];

/** @brief Trace event execution.
 *
 * @param aeh        Pointer to the application event header of the event that is
 *                   processed by app_event_manager.
 * @param is_start  Bool value indicating if this occurrence is related
 *                  to start or end of the event processing.
 **/
static void app_event_manager_trace_event_execution(const struct app_event_header *aeh,
					      bool is_start)
{
	size_t event_cnt = _nrf_profiler_info_list_end - _nrf_profiler_info_list_start;
	size_t event_idx = event_cnt + (is_start ? 0 : 1);
	size_t trace_evt_id = nrf_profiler_event_ids[event_idx];

	if (!IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION) ||
	    !is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;

	ARG_UNUSED(buf);

	nrf_profiler_log_start(&buf);
	nrf_profiler_log_add_mem_address(&buf, aeh);
	nrf_profiler_log_send(&buf, trace_evt_id);
}

static void app_event_manager_trace_event_preprocess(const struct app_event_header *aeh)
{
	app_event_manager_trace_event_execution(aeh, true);
}

static void app_event_manager_trace_event_postprocess(const struct app_event_header *aeh)
{
	app_event_manager_trace_event_execution(aeh, false);
}

APP_EVENT_HOOK_PREPROCESS_REGISTER_FIRST(app_event_manager_trace_event_preprocess);
APP_EVENT_HOOK_POSTPROCESS_REGISTER_LAST(app_event_manager_trace_event_postprocess);

/** @brief Trace event submission.
 *
 * @param aeh Pointer to the application event header of the event that is
 *            submitted to app_event_manager.
 **/
static void app_event_manager_trace_event_submission(const struct app_event_header *aeh)
{
	const void *nrf_profiler_info = aeh->type_id->trace_data;

	if (!nrf_profiler_info) {
		return;
	}

	size_t event_idx = (const struct nrf_profiler_info *) nrf_profiler_info -
							     _nrf_profiler_info_list_start;
	size_t trace_evt_id = nrf_profiler_event_ids[event_idx];

	if (!is_profiling_enabled(trace_evt_id)) {
		return;
	}

	struct log_event_buf buf;

	ARG_UNUSED(buf);

	nrf_profiler_log_start(&buf);

	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_TRACE_EVENT_EXECUTION)) {
		nrf_profiler_log_add_mem_address(&buf, aeh);
	}
	if (IS_ENABLED(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER_PROFILE_EVENT_DATA)) {
		((const struct nrf_profiler_info *)nrf_profiler_info)->profile_fn(&buf, aeh);
	}

	nrf_profiler_log_send(&buf, trace_evt_id);
}

APP_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST(app_event_manager_trace_event_submission);

static void trace_register_execution_tracking_events(void)
{
	static const char * const labels[] = {EM_MEM_ADDRESS_LABEL};
	enum nrf_profiler_arg types[] = {MEM_ADDRESS_TYPE};
	size_t event_cnt = _nrf_profiler_info_list_end - _nrf_profiler_info_list_start;
	uint16_t nrf_profiler_event_id;

	ARG_UNUSED(types);
	ARG_UNUSED(labels);

	/* Event execution start event after last event. */
	nrf_profiler_event_id = nrf_profiler_register_event_type(
				"event_processing_start",
				labels, types, 1);
	nrf_profiler_event_ids[event_cnt] = nrf_profiler_event_id;

	/* Event execution end event. */
	nrf_profiler_event_id = nrf_profiler_register_event_type(
				"event_processing_end",
				labels, types, 1);
	nrf_profiler_event_ids[event_cnt + 1] = nrf_profiler_event_id;
}

static void trace_register_events(void)
{
	STRUCT_SECTION_FOREACH(nrf_profiler_info, pi) {
		size_t event_idx = pi - _nrf_profiler_info_list_start;
		uint16_t nrf_profiler_event_id;

		nrf_profiler_event_id = nrf_profiler_register_event_type(
			pi->name,
			pi->nrf_profiler_arg_labels,
			pi->nrf_profiler_arg_types,
			pi->nrf_profiler_arg_cnt);
		nrf_profiler_event_ids[event_idx] = nrf_profiler_event_id;
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
	/* Every profiled Application Event Manager event registers a single nrf_profiler event.
	 * Apart from that 2 additional nrf_profiler events are used to indicate processing
	 * start and end of an Application Event Manager event.
	 */
	__ASSERT_NO_MSG(_nrf_profiler_info_list_end - _nrf_profiler_info_list_start + 2 <=
			CONFIG_NRF_PROFILER_MAX_NUMBER_OF_APP_EVENTS);

	if (nrf_profiler_init()) {
		LOG_ERR("System nrf_profiler: initialization problem\n");
		return -EFAULT;
	}
	trace_register_events();

	return 0;
}

APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER(app_event_manager_trace_event_init);
