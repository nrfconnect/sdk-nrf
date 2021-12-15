/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AT_MONITOR_H_
#define AT_MONITOR_H_

/**
 * @file at_monitor.h
 *
 * @defgroup at_monitor AT monitor
 *
 * @{
 *
 * @brief Public APIs for the AT monitor library.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <kernel.h>
#include <sys/util_macro.h>
#include <toolchain/common.h>

/**
 * @brief AT monitor callback.
 *
 * @param notif The AT notification.
 */
typedef void (*at_monitor_handler_t)(const char *notif);

/**
 * @brief AT monitor entry.
 */
struct at_monitor_entry {
	/** The filter for this monitor. */
	const char *filter;
	/** Monitor callback. */
	const at_monitor_handler_t handler;
	/** Whether monitor is paused. */
	bool paused;
};

/**
 * @brief AT monitor entry (ISR).
 */
struct at_monitor_isr_entry {
	/** The filter for this monitor. */
	const char *filter;
	/** Monitor callback. */
	const at_monitor_handler_t handler;
	/** Whether monitor is paused. */
	bool paused;
};

/**
 * @brief Ready to dispatch notifications to monitors.
 */
void at_monitor_init(void);

/** Wildcard. Match any notifications. */
#define ANY NULL
/** Monitor is paused. */
#define PAUSED 1
/** Monitor is active, default */
#define ACTIVE 0

/**
 * @brief Define an AT monitor to receive notifications in the system workqueue thread.
 *
 * @param name The monitor name.
 * @param _filter The filter for AT notification the monitor should receive,
 *		  or @c ANY to receive all notifications.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c PAUSED or @c ACTIVE).
 *	      The default initial state of a monitor is active.
 */
#define AT_MONITOR(name, _filter, _handler, ...)                                                   \
	static void _handler(const char *);                                                        \
	STRUCT_SECTION_ITERABLE(at_monitor_entry, at_monitor_##name) = {                           \
		.filter = _filter,                                                                 \
		.handler = _handler,                                                               \
		COND_CODE_1(__VA_ARGS__, (.paused = __VA_ARGS__,), ())                             \
	}

/**
 * @brief Define an AT monitor to receive AT notifications in an ISR.
 *
 * @param name The monitor name.
 * @param _filter The filter for AT notification the monitor should receive,
 *		  or @c ANY to receive all notifications.
 * @param _handler The monitor callback.
 * @param ... Optional monitor initial state (@c PAUSED or @c ACTIVE).
 *	      The default initial state of a monitor is active.
 */
#define AT_MONITOR_ISR(name, _filter, _handler, ...)                                               \
	static void _handler(const char *);                                                        \
	STRUCT_SECTION_ITERABLE(at_monitor_isr_entry, at_monitor_##name) = {                       \
		.filter = _filter,                                                                 \
		.handler = _handler,                                                               \
		COND_CODE_1(__VA_ARGS__, (.paused = __VA_ARGS__,), ())                             \
	}

/**
 * @brief Pause monitor.
 *
 * Pause monitor @p mon from receiving notifications.
 *
 * @param mon The monitor to pause.
 */
#define at_monitor_pause(mon) \
	at_monitor_##mon.paused = 1

/**
 * @brief Resume monitor.
 *
 * Resume forwarding notifications to monitor @p mon.
 *
 * @param mon The monitor to resume.
 */
#define at_monitor_resume(mon) \
	at_monitor_##mon.paused = 0

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* AT_MONITOR_H_ */
