/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

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
	/** Which notification this monitor subscribed to. */
	const char *notif;
	/** Monitor callback. */
	const at_monitor_handler_t handler;
	/** Whether monitor is paused. */
	bool paused;
};

/**
 * @brief Dispatch AT notifications to registered monitors.
 *
 * Notifications are dispatched via the system workqueue.
 *
 * @param notif AT notfication.
 */
void at_monitor_dispatch(const char *notif);

/**
 * @brief Pause monitor.
 *
 * Pause monitor @p mon from receving notifications.
 *
 * @param mon The monitor to pause.
 */
#define at_monitor_pause(mon) at_monitor_##mon.paused = 1

/**
 * @brief Resume monitor.
 *
 * Resume forwarding notifications to monitor @p mon.
 *
 * @param mon The monitor to resume.
 */
#define at_monitor_resume(mon) at_monitor_##mon.paused = 0

/** Wildcard. Match any notifications. */
#define ANY NULL
/** Monitor is paused. */
#define PAUSED 1
/** Monitor is active, default */
#define ACTIVE 0

/**
 * @brief Define an AT monitor.
 *
 * @param name The monitor name.
 * @param _notif The notification the monitor is subscribing for,
 *		 or @c ANY to subcribe to all notifications.
 * @param _fun The monitor callback.
 * @param ... Optional monitor initial state (@c PAUSED or @c ACTIVE).
 *	      The default initial state of a monitor is active.
 */
#define AT_MONITOR(name, _notif, _func, ...)                                   \
	static void _func(const char *);                                       \
	Z_STRUCT_SECTION_ITERABLE(at_monitor_entry, at_monitor_##name) = {     \
		.notif = _notif,                                               \
		.handler = _func,                                              \
		COND_CODE_1(__VA_ARGS__, (.paused = __VA_ARGS__,), ())         \
	}
