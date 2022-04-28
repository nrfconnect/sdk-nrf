/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Application Event Manager profiler tracer header.
 */

#ifndef _APP_EVENT_MANAGER_PROFILER_TRACER_H_
#define _APP_EVENT_MANAGER_PROFILER_TRACER_H_


/**
 * @defgroup app_event_manager_profiler_tracer Application Event Manager profiler tracer
 * @brief Application Event Manager profiler tracer
 *
 * @{
 */

#include <app_event_manager_profiler_tracer_priv.h>
#include <nrf_profiler.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Event description for profiling.
 */
struct nrf_profiler_info {
	/** Function for profiling data related with this event */
	void (*profile_fn)(struct log_event_buf *buf, const struct app_event_header *aeh);

	/** Number of profiled data fields. */
	const uint8_t nrf_profiler_arg_cnt;

	/** Labels of profiled data fields. */
	const char **nrf_profiler_arg_labels;

	/** Types of profiled data fields. */
	const enum nrf_profiler_arg *nrf_profiler_arg_types;

	/** Profiled event name. */
	const char *name;
};


/** Encode event data types or labels.
 *
 * @param ... Data types or labels to be encoded.
 */
#define ENCODE(...) __VA_ARGS__


/** Define event profiling information.
 *
 * This macro provides definitions required for an event to be profiled.
 *
 * @note Types and labels of the profiled values should be wrapped
 *       with the @ref ENCODE macro.
 *
 * @param ename Name of the event.
 * @param types Types of values to profile (represented as @ref nrf_profiler_arg).
 * @param labels Labels of values to profile.
 * @param profile_func Function used to profile event data.
 */
#define APP_EVENT_INFO_DEFINE(ename, types, labels, profile_func) \
	BUILD_ASSERT(profile_func != NULL);			\
	_APP_EVENT_INFO_DEFINE(ename, ENCODE(types), ENCODE(labels), profile_func)



#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _APP_EVENT_MANAGER_PROFILER_TRACER_H_ */
