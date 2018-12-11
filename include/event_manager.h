/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/** @file
 * @brief Event manager header.
 */

#ifndef _EVENT_MANAGER_H_
#define _EVENT_MANAGER_H_


/**
 * @brief Event Manager
 * @defgroup event_manager Event Manager
 *
 * The event manager module provides an infrastructure required for the
 * implementation using an event-driven architecture approach.
 *
 * When defining a new event type, user must provide definition of this event
 * type structure, that contains at least one member 'header' of type
 * @ref struct event_header. If more members are provided 'header' must be first
 * in the event type structure.
 *
 * New event type is defined by using @ref EVENT_TYPE_DEFINE. The only
 * argument passed to this macro is the name of the structure declared in
 * previous paragraph.
 * This macro will expand into definition of new element in array of event
 * types, and define various functions required for its usage.
 *
 * When above is done user can create and submit events of this new type.
 * The new event object is created by using function defined by macro
 * new_'event type name' (e.g. new_motion_event). If there is no memory
 * available for event allocation reset is triggered.
 * After new event object is created, its field can be filled in by the user.
 * Event is submitted to the event manager using a @ref EVENT_SUBMIT macro.
 *
 * Note: an event allocated with new__'event type name' MUST be submitted with
 * @ref EVENT_SUBMIT. Otherwise it will not be handled and the memory will
 * not be freed.
 *
 * After event is submitted the event manager adds it into its processing
 * queue. When event is handled the event manager will notify all modules
 * that subscribed for its reception.
 * A module willing to subscribe for a reception of events must first define
 * itself as event listener using a @ref EVENT_LISTENER macro. First argument
 * should be module name, while second is a callback function called for
 * each event being processed.
 * Event listener can subscribe to events of specific type using either:
 * @ref EVENT_SUBSCRIBE_EARLY, @ref EVENT_SUBSCRIBE, @ref EVENT_SUBSCRIBE_FINAL.
 * Early subscribers are notified first, before any other. There is no
 * defined order in which subscribers of the same time are notified.
 * Only one listener can be final subscriber for any event type, and if so
 * it will be notified last, after all other modules subscribed for that
 * event.
 *
 * Single listener can be subscribed to events of multiple types. The same
 * callback function is called when any of subscribed events is being processed.
 * To check type of incoming event user should use macro defined function
 * is_'event type name' (e.g. is_motion_event). Function accepts pointer event
 * header which is an argument to event handling function and returns true
 * is event type matches.
 *
 * @{
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <misc/reboot.h>
#include <misc/__assert.h>

#include <event_manager_priv.h>
#include <profiler.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @def SUBS_PRIO_MIN
 *
 * @brief Index of the highest subscriber priority level.
 */
#define SUBS_PRIO_MIN    _SUBS_PRIO_FIRST


/** @def SUBS_PRIO_MAX
 *
 * @brief Index of the lowest subscriber priority level.
 */
#define SUBS_PRIO_MAX    _SUBS_PRIO_FINAL


/** @def SUBS_PRIO_COUNT
 *
 * @brief Number of subscriber priority levels.
 */
#define SUBS_PRIO_COUNT (SUBS_PRIO_MAX - SUBS_PRIO_MIN + 1)


/** @brief Event header structure.
 *
 * @warning When event structure is defined event header must be placed
 *          as the first field.
 */
struct event_header {
	/** Linked list node used to chain events. */
	sys_dlist_t node;

	/** Pointer to the event type object. */
	const struct event_type *type_id;
};


/** @brief Event listener structure.
 *
 * @note All event listeners must be defined using @ref EVENT_LISTENER.
 */
struct event_listener {
	/** Name of this listener. */
	const char *name;

	/** Pointer to function that is called when event is handled. */
	bool (*notification)(const struct event_header *eh);
};


/** @brief Event subscriber structure.
 */
struct event_subscriber {
	/** Pointer to the listener. */
	const struct event_listener *listener;
};


/** @brief Event description for profiling or logging.
 */
struct event_info {
	/** Function to profile this event. */
	void (*profile_fn)(struct log_event_buf *buf,
			   const struct event_header *eh);

	/** Number of logged datafields. */
	const u8_t log_arg_cnt;

	/** Labels of logged datafields. */
	const char **log_arg_labels;

	/** Types of logged datafields. */
	const enum profiler_arg *log_arg_types;
};


/** @brief Event type structure.
 */
struct event_type {
	/** Event name. */
	const char			*name;

	/** Array of pointers to the array of subscribers. */
	const struct event_subscriber	*subs_start[SUBS_PRIO_COUNT];

	/** Array of pointers to element directly after the array of
	 * subscribers. */
	const struct event_subscriber	*subs_stop[SUBS_PRIO_COUNT];

	/** Function to log data from this event. */
	int (*log_event)(const struct event_header *eh, char *buf,
			      size_t buf_len);

	/** Logging and formatting information. */
	const struct event_info *ev_info;
};


extern const struct event_listener __start_event_listeners[];
extern const struct event_listener __stop_event_listeners[];

extern const struct event_type __start_event_types[];
extern const struct event_type __stop_event_types[];


/** @def EVENT_LISTENER
 *
 * @brief Create event listener object.
 *
 * @param lname   Module name.
 * @param cb_fn  Pointer to the event notification function.
 */
#define EVENT_LISTENER(lname, cb_fn) _EVENT_LISTENER(lname, cb_fn)


/** @def EVENT_SUBSCRIBE_EARLY
 *
 * @brief Subscribe listener to the event type early notification list.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_EARLY(lname, ename) \
	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_FIRST))


/** @def EVENT_SUBSCRIBE
 *
 * @brief Subscribe listener to the event type to normal notification list.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE(lname, ename) \
	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_NORMAL))


/** @def EVENT_SUBSCRIBE_FINAL
 *
 * @brief Subscribe listener to the event type as final module being notified.
 *
 * @param lname  Name of the listener.
 * @param ename  Name of the event.
 */
#define EVENT_SUBSCRIBE_FINAL(lname, ename)							\
	_EVENT_SUBSCRIBE(lname, ename, _SUBS_PRIO_ID(_SUBS_PRIO_FINAL));			\
	const struct {} _CONCAT(_CONCAT(__event_subscriber_, ename), final_sub_redefined) = {}


/** @def ENCODE
 *
 * @brief Encode event data types or labels.
 *
 * @param ... Data types or labels to be encoded.
 */
#define ENCODE(...) __VA_ARGS__


/** @def EVENT_INFO_DEFINE
 *
 * @brief Declare event logging information.
 *
 * Macro provides declarations required for event to be logged by profiler.
 *
 * @param ename Name of the event.
 * @param types Types of values to log.
 * @param labels Labels of values to log (represented as enum).
 * @param log_arg_func Function used to log event data.
 */
#define EVENT_INFO_DEFINE(ename, types, labels, profile_func) \
	_EVENT_INFO_DEFINE(ename, ENCODE(types), ENCODE(labels), profile_func)


/** @def EVENT_TYPE_DECLARE
 *
 * @brief Declare event type.
 *
 * Macro provides declarations required for event to be used by other modules.
 *
 * @param ename  Name of the event.
 */
#define EVENT_TYPE_DECLARE(ename) _EVENT_TYPE_DECLARE(ename)


/** @def EVENT_TYPE_DEFINE
 *
 * @brief Define event type.
 *
 * Macro defines the event type. By doing that it defines the event type
 * specific functions as well event type structure.
 *
 * @param ename     		Name of the event.
 * @param print_fn  		Function to stringify event of this type.
 * @param ev_info_struct	Data structure describing event type.
 */
#define EVENT_TYPE_DEFINE(ename, log_fn, ev_info_struct) \
	_EVENT_TYPE_DEFINE(ename, log_fn, ev_info_struct)


/** @def ASSERT_EVENT_ID
 *
 * @brief Verify if event id is valid.
 *
 * Pointer to event type structure is used as its id. Macro validates that
 * provided pointer is withing range where event type structures are defined.
 *
 * @param id
 */
#define ASSERT_EVENT_ID(id) \
	__ASSERT_NO_MSG((id >= __start_event_types) && (id < __stop_event_types))


/**
 * @brief Submit an event.
 *
 * Function submits an event to the event manager.
 *
 * @param eh  Pointer to the event header element in the event object.
 */
void _event_submit(struct event_header *eh);


/** @def EVENT_SUBMIT
 *
 * @brief Submit an event.
 *
 * This is a helper macro that simplifies the event submission.
 *
 * @param event  Pointer to the event object.
 */
#define EVENT_SUBMIT(event) _event_submit(&event->header)


/** Initialize the event manager.
 *
 * @return Zero if successful.
 */
int event_manager_init(void);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EVENT_MANAGER_H_ */
