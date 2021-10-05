/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief Event Manager Profiler header.
 */

#ifndef _EVENT_MANAGER_PROFILER_H_
#define _EVENT_MANAGER_PROFILER_H_


/**
 * @addtogroup event_manager Event Manager
 * @brief Event Manager
 *
 * @{
 */

#include <event_manager_profiler_priv.h>
#include <profiler.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Event description for profiling.
 */
struct profiler_info {
	/** Function for profiling data related with this event */
	void (*profile_fn)(struct log_event_buf *buf, const struct event_header *eh);

	/** Number of profiled data fields. */
	const uint8_t profiler_arg_cnt;

	/** Labels of profiled data fields. */
	const char **profiler_arg_labels;

	/** Types of profiled data fields. */
	const enum profiler_arg *profiler_arg_types;

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
 * @param types Types of values to profile (represented as @ref profiler_arg).
 * @param labels Labels of values to profile.
 * @param profile_func Function used to profile event data.
 */
#define EVENT_INFO_DEFINE(ename, types, labels, profile_func) \
	BUILD_ASSERT(profile_func != NULL);			\
	_EVENT_INFO_DEFINE(ename, ENCODE(types), ENCODE(labels), profile_func)



#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EVENT_MANAGER_PROFILER_H_ */
